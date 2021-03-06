#include "Score.h"

using namespace std;

Score::Score() : tempo(500000), track(1), time_parse(0), note_parse(0),id_parse(0){}

Score::Score(string input_ust):tempo(500000),track(1),time_parse(0),note_parse(0),id_parse(0)
{
  loadUst(input_ust);
}

Score::Score(string singer, string input_smf, short track, string path_lyric, string path_song)
  :tempo(500000),track(1),time_parse(0),note_parse(0),id_parse(0)
{
  loadSmf(input_smf, track, path_lyric);
  setSinger(singer);
  setSongPath(path_song);
}

Score::~Score()
{
  if (nak::log)
    cout << "----- finish score loading -----" << endl;
}

bool Score::isScoreLoaded()
{
  return !notes.empty();
}

void Score::loadSmf(string input, unsigned short track, string path_lyric)
{
  if (nak::log)
    cout << "----- start score(smf) loading -----" << endl;

  // load lyric txt
  list<string> prons;
  ifstream ifs(path_lyric.c_str());
  string buf_str;

  while (ifs && getline(ifs, buf_str)) {
    if (buf_str == "")
      continue;
    if (*(buf_str.end()-1) == ',')
      buf_str.erase(buf_str.end()-1,buf_str.end());
    vector<string> buf_vector;
    boost::algorithm::split(buf_vector, buf_str, boost::is_any_of(","));
    prons.insert(prons.end(), buf_vector.begin(), buf_vector.end());
  }

  // load smf
  this->track = track;
  SmfParser *smf_parser = new SmfParser(input);
  if (smf_parser->isSmfFile()) {
    smf_parser->addSmfHandler(this);
    smf_parser->parse();
    is_parse = false;
  } else {
    ifstream ifs;
    ifs.open(input.c_str(), ios::in|ios::binary);
    if (!ifs) {
      cerr << "[Score::loadSmf] " << input << " cannot open\n";
      return;
    }
    Note *note = 0;
    while (!ifs.eof()) {
      ifs.read((char *)note, sizeof(Note));
      notes.push_back(*note);
    }
  }
  if (notes.size() == 0) {
    cerr << "[Score::loadSmf] cannot read notes" << endl;
    return;
  }
  list<Note>::iterator it_notes = notes.begin();
  list<string>::iterator it_prons = prons.begin();
  for (; it_notes!=notes.end()&&it_prons!=prons.end(); ++it_notes,++it_prons)
    (*it_notes).setPron(*it_prons);

  if (pitches.empty())
    reloadPitches();
}

void Score::loadUst(string path_ust)
{
  if (nak::log)
    cout << "----- start score(ust) loading -----" << endl
      << "ust: " << path_ust << endl;

  // read ust
  ifstream ifs(path_ust.c_str());
  string buf_str;
  list<string> buf_list;
  short tmp, tempo=120;
  unsigned long pos=0;
  notes.clear();
  pitches.clear();
  while (ifs && getline(ifs, buf_str)) {
    if (buf_str == "[#SETTING]")
      continue;
    if (buf_str[0]=='[') {
      Note *tmp_note = new Note(this, ++id_parse);
      if (notes.size()>0) {
        tmp_note->setStart(notes.back().getEnd());
        if (notes.back().getPron()=="R")
          notes.pop_back();
      }
      notes.push_back(*tmp_note);
      continue;
    }
    vector<string> buf_vector;
    boost::algorithm::split(buf_vector, buf_str, boost::is_any_of("="));
    if (buf_vector[0] == "Tempo")
      tempo = (buf_vector[1]!="" && ((tmp=boost::lexical_cast<double>(buf_vector[1]))>0))?tmp:0;
    if (buf_vector[0] == "VoiceDir") {
      boost::algorithm::replace_all(buf_vector[1], "%", "/");
      if (buf_vector[1][0] != '/')
        buf_vector[1] = "/" + buf_vector[1];
      setSinger(".."+buf_vector[1]);
    }
    if (buf_vector[0] == "OutFile") {
      boost::algorithm::replace_all(buf_vector[1], "%", "/");
      if (buf_vector[1][0] != '/')
        buf_vector[1] = "/" + buf_vector[1];
      setSongPath("."+buf_vector[1]);
    }
    if (buf_vector[0] == "Length")
      if (buf_vector[1]!="" && (tmp=boost::lexical_cast<short>(buf_vector[1]))>0)
        notes.back().setEnd(pos+=tmp, 480, 1.0/tempo*60000000);
    if (buf_vector[0] == "Lyric")
      notes.back().setPron(buf_vector[1]);
    if (buf_vector[0] == "NoteNum")
      if (buf_vector[1]!="" && (tmp=boost::lexical_cast<short>(buf_vector[1]))>0)
        notes.back().setBasePitch(tmp);
    if (buf_vector[0] == "PreUtterance")
      if (buf_vector[1]!="")
        notes.back().setPrec(boost::lexical_cast<short>(buf_vector[1]));
    if (buf_vector[0] == "VoiceOverlap")
      if (buf_vector[1]!="")
        notes.back().setOvrl(boost::lexical_cast<short>(buf_vector[1]));
    if (buf_vector[0] == "Intensity")
      if (buf_vector[1]!="" && (tmp=boost::lexical_cast<short>(buf_vector[1]))>0)
        notes.back().reloadVelocities(tmp);
  }
  while (notes.back().getPron()=="R" || notes.back().getPron()=="")
    notes.pop_back();

  if (pitches.empty())
    reloadPitches();
}

void Score::reloadPitches()
{
  pitches.clear();
  pitches.resize(notes.back().getEnd(), 0.0);
  for (list<Note>::iterator it=notes.begin(); it!=notes.end(); ++it)
    for (int i=it->getStart(); i<it->getEnd(); i++)
      pitches[i] = it->getBasePitchHz();
}

void Score::debug(string output)
{
  ofstream ofs;
  ofs.open(output.c_str());

  ofs << setw(8) << "timebase" << setw(8) << timebase << endl
      << setw(8) << "tempo" << setw(8) << tempo << endl << endl << endl;
  ofs << setw(8) << "start"
      << setw(8) << "length"
      << setw(6) << "pit"
      << setw(6) << "vel" << endl << endl;
  for (list<Note>::iterator it=notes.begin(); it!=notes.end(); ++it)
    ofs << setw(8) << dec << it->getStart()
        << setw(8) << it->getEnd()
        << setw(6) << hex << (unsigned int)it->getBasePitch()
        << setw(6) << (unsigned int)it->getVelocities()[0] << endl;
}


/*
 * Note mediator
 */
void Score::noteParamChanged(Note *note)
{
  if (notes.size() == 0)
    return;
  list<Note>::iterator it_tmp_note=find(notes.begin(), notes.end(), *note);
  if (it_tmp_note!=notes.begin()) {
    (boost::prior(it_tmp_note))->setLack(note->getPrec()-note->getOvrl());
    (boost::prior(it_tmp_note))->reloadVelocities();
  }

  note->reloadVelocities();
}


/*
 * accessor
 */
string Score::getSinger()
{
  return singer;
}

void Score::setSinger(string singer)
{
  this->singer = singer;
}

string Score::getSongPath()
{
  return path_song;
}

void Score::setSongPath(string path_song)
{
  this->path_song = path_song;
}

vector<float> Score::getPitches()
{
  return pitches;
}

void Score::setPitches(vector<float> pitches)
{
  this->pitches = pitches;
}


/*
 * inherit from SmfParser 
 */
void Score::smfInfo(short numTrack, short timebase)
{
  this->timebase = timebase;
}

void Score::trackChange(short track)
{
  time_parse = 0;
  is_parse = (this->track == track);
}

void Score::eventMidi(long deltatime, unsigned char msg, unsigned char* data)
{
  if (!is_parse)
    return;

  time_parse += deltatime;

  if (SmfHandler::charToMidiMsg(msg) == MIDI_MSG_NOTE_ON) {
    if (note_parse) {
      if (note_parse->getBasePitch() == data[0]) {
        if (data[1] == 0) {
          note_parse->setEnd(time_parse, timebase, tempo);
          notes.push_back(*note_parse);
          delete note_parse;
          note_parse = NULL;
        } else {
          return;
        }
      } else {
        if (data[1] == 0) {
          return;
        } else {
          note_parse->setEnd(time_parse, timebase, tempo);
          notes.push_back(*note_parse);
          delete note_parse;
          note_parse = new Note(this, ++id_parse, 0, timebase, tempo, data[0], data[1]);
        }
      }
    } else {
      note_parse = new Note(this, ++id_parse, time_parse, timebase, tempo, data[0], data[1]);
    }
  } else if (SmfHandler::charToMidiMsg(msg) == MIDI_MSG_NOTE_OFF && note_parse){
    note_parse->setEnd(time_parse, timebase, tempo);
    notes.push_back(*note_parse);
    delete note_parse;
    note_parse = NULL;
  }
}

void Score::eventSysEx(long deltatime, long datasize, unsigned char* data){
  if (is_parse)
    time_parse += deltatime;
}

void Score::eventMeta(long deltatime, unsigned char type, long datasize, unsigned char* data)
{
  if (is_parse)
    time_parse += deltatime;

  if (type == 0x51 && datasize == 3) {
    tempo = data[0] << 16;
    tempo += data[1] << 8;
    tempo += data[2];
  }
}
