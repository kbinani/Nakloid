#include "PitchMarker.h"

#pragma comment(lib, "libfftw3-3.lib")

using namespace std;

PitchMarker::PitchMarker():pitch(0),cons_pos(0){}

PitchMarker::PitchMarker(short win_size):pitch(0),cons_pos(0)
{
  setWinSize(win_size);
}

PitchMarker::PitchMarker(short pitch, unsigned long fs):cons_pos(0)
{
  setPitch(pitch, fs);
}

PitchMarker::~PitchMarker(){}

short PitchMarker::getPitch()
{
  return this->pitch;
}

void PitchMarker::setPitch(short pitch, unsigned long fs)
{
  this->pitch = pitch;
  setWinSize((short)(fs / pitch));
}

short PitchMarker::getWinSize()
{
  return this->win_size;
}

void PitchMarker::setWinSize(short win_size)
{
  this->win_size = win_size;
}

list<long> PitchMarker::getMarkList()
{
  return this->mark_list;
}

vector<long> PitchMarker::getMarkVector()
{
  vector<long> mark_vector(mark_list.size());

  int i = -1;
  for (list<long>::iterator it=mark_list.begin(); it!=mark_list.end(); ++it)
    mark_vector[++i] = *it;

  return mark_vector;
}

long PitchMarker::getConsPos()
{
  return cons_pos;
}

void PitchMarker::setConsPos(unsigned short cons, unsigned long fs)
{
  this->cons_pos = fs / 1000.0 * cons;
}

bool PitchMarker::mark(WavData input)
{
  return mark(input.getDataVector());
}

bool PitchMarker::mark(list<short> input)
{
  vector<short> input_vector(input.size(), 0);

  long i = -1;
  for (list<short>::iterator it=input.begin(); it!=input.end(); ++it)
    input_vector[++i] = *it;

  return mark(input_vector);
}

bool PitchMarker::mark(vector<short> input)
{
  if (win_size <= 0)
    return false;
  if (nak::log)
    cout << "----- start pitch marking -----" << endl;

  vector<short>::iterator mark_next, mark_prev;
  mark_list.clear();

  // find base mark
  vector<short>::iterator it_start;
  if (cons_pos == 0) {
    vector<short>::iterator it_max = max_element(input.begin()+win_size+1, input.end()-win_size-1);
    vector<short>::iterator it_min = min_element(input.begin()+win_size+1, input.end()-win_size-1);
    it_start = (*it_max>-*it_min)?it_max:it_min;
  } else {
    vector<short>::iterator it_max = max_element(input.begin()+cons_pos, input.begin()+cons_pos+win_size);
    vector<short>::iterator it_min = min_element(input.begin()+cons_pos, input.begin()+cons_pos+win_size);
    it_start = (*it_max>-*it_min)?it_max:it_min;
  }
  vector<double> filter = nak::getHann(win_size/2);
  double max_point = 0.0;
  vector<short>::iterator it_tmp_max;
  for (int i=-win_size/4; i<win_size/4; i++) {
    if (it_start-input.begin()<-i || i>input.end()-1-it_start)
      continue;
    vector<short> tmp_input(it_start+i-(win_size/4), it_start+i+(win_size/4));
    double tmp_max_point = 0.0;
    double tmp_mean = nak::getMean(tmp_input);
    for (int j=0; j<min(tmp_input.size(),filter.size()); j++)
      tmp_max_point += pow((tmp_input[j]-tmp_mean)*filter[j], 2);
    if (tmp_max_point > max_point) {
      max_point = tmp_max_point;
      it_tmp_max = it_start + i;
    }
  }
  it_start = it_tmp_max;
  mark_list.push_back((mark_prev=mark_next=it_start)-input.begin());
  if (nak::log)
    cout << "win_size:" << win_size << ", start:" << it_start-input.begin() << ", input.size:" << input.size() << endl;

  // pitch marking
  while (input.end()-mark_next > win_size*1.5) {
    vector<double> xcorr_win = xcorr(it_start, mark_next, win_size);
    unsigned short pitch_margin = max((unsigned short)(xcorr_win.size()/2), nak::pitch_margin);
    long dist = max_element(xcorr_win.begin()+(xcorr_win.size()/2)-pitch_margin,
      xcorr_win.begin()+(xcorr_win.size()/2)+pitch_margin)-xcorr_win.begin();
    mark_list.push_back((mark_next+=dist)-input.begin());
  }
  if (input.end()-mark_next > win_size)
    mark_list.push_back(mark_next+win_size-input.begin());

  long dist = *(++mark_list.begin())-mark_list.front();
  while (mark_prev-input.begin()>max(dist,(long)win_size)*1.5 && dist>0) {
    int tmp = mark_list.size();
    vector<double> xcorr_win = xcorr(mark_prev+dist, mark_prev, -win_size);
    unsigned short pitch_margin = min((unsigned short)(xcorr_win.size()/2), nak::pitch_margin);
    dist = max_element(xcorr_win.begin()+(xcorr_win.size()/2)-pitch_margin,
      xcorr_win.begin()+(xcorr_win.size()/2)+pitch_margin)-xcorr_win.begin();
    if (mark_prev-input.begin() < dist)
      break;
    else
      mark_list.push_front((mark_prev-=dist)-input.begin());
  }
  if (mark_prev-input.begin() > max(dist,(long)win_size))
    mark_list.push_front(mark_prev-max(dist,(long)win_size)-input.begin());
  mark_list.push_front(0);

  if (nak::log)
    cout << "----- finish pitch marking -----" << endl << endl;
  return true;
}

// cross correlation
vector<double> PitchMarker::xcorr(vector<short>::iterator it_start, vector<short>::iterator it_base, short exp_dist)
{
  int win_size = exp_dist * ((exp_dist>0)?1:-1);
  int fftlen = win_size * 2;
  vector<double> filter = getHann(win_size);

  fftw_complex *in1 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));
  fftw_complex *in2 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));
  fftw_complex *in3 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));
  fftw_complex *out1 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));
  fftw_complex *out2 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));
  fftw_complex *out3 = (fftw_complex*)(fftw_malloc(sizeof(fftw_complex) * fftlen));

  for (int i=0; i<fftlen; i++)
    in1[i][0] = in1[i][1] = in2[i][0] = in2[i][1] = 0;  if (exp_dist > 0) {
  for (int i=0; i<win_size; i++) {
      in1[i][0] = (*(it_start-(win_size/2)+i)) * filter[i];
      in2[i+win_size][0] = *(it_base+(win_size/2)+i) * filter[i];
    }
  } else {
    for (int i=0; i<win_size; i++) {
      in1[i][0] = *(it_start+(win_size/2)-i) * filter[i];
      in2[i+win_size][0] = *(it_base-(win_size/2)-i) * filter[i];
    }
  }

  fftw_plan p1 = fftw_plan_dft_1d(fftlen, in1, out1, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p1);
  fftw_destroy_plan(p1);

  fftw_plan p2 = fftw_plan_dft_1d(fftlen, in2, out2, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p2);
  fftw_destroy_plan(p2);

  for (int i=0; i<fftlen; i++) {
    in3[i][0] = (out1[i][0]*out2[i][0])+(out1[i][1]*out2[i][1]);
    in3[i][1] = (out1[i][0]*out2[i][1])-(out1[i][1]*out2[i][0]);
  }

  fftw_plan p3 = fftw_plan_dft_1d(fftlen, in3, out3, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_execute(p3);
  fftw_destroy_plan(p3);

  vector<double> output(fftlen, 0);
  for (int i=0; i<fftlen; i++)
    output[i] = out3[i][0];

  fftw_free(in1);
  fftw_free(in2);
  fftw_free(in3);
  fftw_free(out1);
  fftw_free(out2);
  fftw_free(out3);

  return output;
}

vector<double> PitchMarker::getHann(long len)
{
  vector<double> filter(len, 0);

  for (int i=0; i<filter.size(); ++i) {
    double x = (i+1.0) / (filter.size()+1.0);
    filter[i] = 0.5 - (0.5 * cos(2*M_PI*x));
  }

  return filter;
}
void PitchMarker::debug(string output)
{
  if (mark_list.empty())
    return;

  ofstream ofs(output.c_str());
  for (list<long>::iterator it=mark_list.begin(); it!=mark_list.end(); ++it){
    ofs << setw(8) << *it << endl;
  }
}
