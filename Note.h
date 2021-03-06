#ifndef Note_h
#define Note_h

#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include "Score.h"
class Score;

typedef struct {
  unsigned long start;
  unsigned long end;
  std::string pron;
  unsigned char base_pitch;
  short base_velocity;
  std::vector<short> velocities;
  short lack;
  short *prec;
  short *ovrl;
} NoteFrame;

// Value Object
class Note {
 public:
  Note(Score *score, unsigned long id);
  Note(Score *score, unsigned long id, unsigned long deltatime, unsigned short timebase, unsigned long tempo, unsigned char base_pitch, short base_velocity);
  Note(const Note& other);
  ~Note();

  Note& operator=(const Note& other);
  bool operator==(const Note& other) const;
  bool operator!=(const Note& other) const;

  // accessor
  unsigned long getStart();
  unsigned long getPronStart();
  void setStart(unsigned long ms_start);
  void setStart(unsigned long deltatime, unsigned short timebase, unsigned long tempo);
  unsigned long getEnd();
  unsigned long getPronEnd();
  void setEnd(unsigned long ms_end);
  void setEnd(unsigned long deltatime, unsigned short timebase, unsigned long tempo);
  std::string getPron();
  void setPron(std::string pron);
  unsigned char getBasePitch();
  double getBasePitchHz();
  void setBasePitch(unsigned char base_pitch);
  void reloadVelocities();
  void reloadVelocities(short base_velocity);
  short getBaseVelocity();
  std::vector<short> getVelocities();
  void setVelocities(std::vector<short> velocities);
  short getLack();
  void setLack(short lack);
  bool isPrec();
  short getPrec() const;
  void setPrec(short prec);
  bool isOvrl();
  short getOvrl() const;
  void setOvrl(short ovrl);

 private:
  Score *score;
  unsigned long id;
  NoteFrame self;

  void initializeNoteFrame();
  unsigned long tick2ms(unsigned long tick, unsigned short timebase, unsigned long tempo);
};

#endif
