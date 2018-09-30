//‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
//›                                                                         ﬁ
//› Module: Header file for stock car rules                                 ﬁ
//›                                                                         ﬁ
//› Description: Implements stock car rules for vehicle races               ﬁ
//›                                                                         ﬁ
//› This source code module, and all information, data, and algorithms      ﬁ
//› associated with it, are part of isiMotor Technology (tm).               ﬁ
//›                 PROPRIETARY AND CONFIDENTIAL                            ﬁ
//› Copyright (c) 1996-2015 Image Space Incorporated.  All rights reserved. ﬁ
//›                                                                         ﬁ
//› Change history:                                                         ﬁ
//›   tag.2015.09.30: created                                               ﬁ
//›                                                                         ﬁ
//ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ

#ifndef _STOCK_CAR_RULES_HPP_
#define _STOCK_CAR_RULES_HPP_

#include "InternalsPlugin.hpp"     // base class for plugin objects to derive from
#include <cstdio>                  // for logging


class StockCarRulesPlugin : public InternalsPluginV07 // REMINDER: exported function GetPluginVersion() should return 1 if you are deriving from this InternalsPluginV01, 2 for InternalsPluginV02, etc.
{
 protected:

  // logging
  bool mLogging;                        // whether we want logging enabled
  char *mLogPath;                       // path to write logs to
  FILE *mLogFile;                       // the file pointer

  // behavior controls
  float mAdjustFrozenOrder;             // a threshold (as a fraction of the safety car threshold) in order to demote vehicles down the frozen order
  long mAllowFrozenAdjustments;         // 0 = don't allow any adjustments, 1 = allow adjustments when somebody is passed while causing yellow, 2 = adjust for any pass (essentially, no frozen order)
  long mAdjustUntilYellowFlagState;     // which yellow flag state to stop adjusting order at
  bool mEnableLuckyDogFreePass;         // whether lucky dog/free pass is enabled

  // participant info
  struct KnownCar
  {
    // basic info or info derived while processing track rules
    long mIndex;                        // index into mKnownCar array
    long mID;                           // slot ID
    long mLapsDown;                     // number of laps down when caution came out (0 = on the lead lap)
    float mWorstYellowSeverity;         // worst yellow severity (a rating of how much this vehicle is causing a yellow) around the time the full-course caution was called
    bool mPittedAtDesignatedTime;       // whether vehicle pitted at the designated time (depending on whether it was on the lead lap)
    bool mPittedAtSomeOtherTime;        // whether vehicle pitted at some other time

    // info copied from latest scoring update
    bool mUnderYellow;                  // whether vehicle has taken full-course caution at s/f line
    bool mInPits;                       // between pit entrance and pit exit (not always accurate for remote vehicles)
    unsigned char mPitState;            // 0=none, 1=request, 2=entering, 3=stopped, 4=exiting
    unsigned char mCountLapFlag;        // 0 = do not count lap or time, 1 = count lap but not time, 2 = count lap and time
    signed char mFinishStatus;          // 0=none, 1=finished, 2=dnf, 3=dq
    unsigned char mPlace;               // 1-based position (current place in race)
    short mTotalLaps;                   // laps completed
    unsigned char mNumEOLLs;            // number of "End Of Longest Line" penalties

    char mDriverName[ sizeof( VehicleScoringInfoV01 ) ];    // driver name (this could change during a swap)
    double mPathLateral;                // lateral position w.r.t. the approximate "center" of track
    double mLapDist;                    // current lap distance

    // Total lap distance, with a correction for rolling starts, where mTotalLaps remains zero during the first
    // s/f crossing after green. This can be detected because the "count lap flag" changes from 0 to 2.
    double CorrectedTotalLapDist( double fullLapDist ) const
    {
      double ret = ( mTotalLaps * fullLapDist ) + mLapDist;
      if( ( 0 == mTotalLaps ) && ( 0 == mCountLapFlag ) )
        ret -= fullLapDist;
      return( ret );
    }
  };

  long mMaxKnownCars;                   // number of allocated known cars
  long mNumKnownCars;                   // number of valid known cars
  KnownCar *mKnownCar;                  // array of known cars

  enum PassStatus : unsigned char // this works in VC, but if there's trouble with other compilers one can also use the C++11 construct: enum class ObsticalePlans : unsigned char
  {
    PS_NONE = 0,                        // no pass recorded
    PS_PHYSICAL,                        // physically passed, but not currently legal
    PS_LEGAL                            // a legal pass, but *maybe* pending because some other illegal pass involving a third car has not been resolved    
  };
  PassStatus *mFrozenPass;              // 2-D array of the status of on-track passes under yellow (see code for usage)

  // track order in each lane (which is recorded and frozen when caution starts, and then adjusted and maintained throughout the caution period)
  struct Lane
  {
    struct Member
    {
      long mID;                         // slot ID
      double mDist;                     // total lap dist when a caution came out and the field was frozen
    };

    long mMaxMembers;                   // number of allocated members
    long mNumMembers;                   // number of valid members
    Member *mMember;                    // array of members

    //
    Lane()                             { mMaxMembers = 0; mNumMembers = 0; mMember = NULL; }
    ~Lane()                            { delete [] mMember; }
    void AddMember( const Member &member )
    {
      if( mNumMembers >= mMaxMembers )
      {
        Member *newMember = new Member[ ++mMaxMembers ];
        for( long i = 0; i < mNumMembers; ++i )
          newMember[ i ] = mMember[ i ];
        delete [] mMember;
        mMember = newMember;
      }
      mMember[ mNumMembers++ ] = member;
    }
  };

  Lane mLane[ TRCOL_MAX_LANES ];        // array of lanes (but the stock car implementation only uses TRCOL_LEFT_LANE and TRCOL_RIGHT_LANE)

  // store some actions to apply a little later (when we are sure of the correct order, accounting for possible net lag)
  long mMaxActions;                     // number of allocated actions
  long mNumActions;                     // number of valid actions
  TrackRulesActionV01 *mAction;         // array of actions

  // some state info
  bool mProcessedLastLapOfCaution;      // Whether we have processed the last caution lap yet
  long mLuckyDogID;                     // Keep track of who got the lucky dog if anybody (-1 if not applied yet)
  double mLeaderLaneChoiceLateral;      // Lateral position of leader when first requested to make a lane choice (or DBL_MAX if invalid)
  long mYellowFlagLaps;                 // Intended number of yellow flag laps to complete
  double mCautionET;                    // time at which the caution was initialized
  float mRandomGreenDist;               // A random value to prevent green flag from being shown at exactly the same spot every time

  // info copied from the latest scoring update
  long mSession;                        // current session (0=testday 1-4=practice 5-8=qual 9=warmup 10-13=race)
  double mCurrentET;                    // current time
  double mEndET;                        // ending time
  long  mMaxLaps;                       // maximum laps
  double mFullLapDist;                  // distance around a full lap of the track
  unsigned char mGamePhase;             // see InternalsPlugin.hpp
  signed char mYellowFlagState;         // see InternalsPlugin.hpp

  // protected method(s)
  void Clear();
  void AddMember( TrackRulesColumnV01 colIndex, const Lane::Member &member );
  bool SafetyCarIsApproachingPitlane( TrackRulesV01 &info, long numLaps );
  void ApplyLuckyDogFreePass( TrackRulesV01 &info );
  const KnownCar *GetFirstCarInRightLaneThatIsOnTheLeadLap() const;

  static int _cdecl CompareFrozen( const void *slot1, const void *slot2 );
  void FreezeOrder( TrackRulesV01 &info );
  void AdjustFrozenOrderNew( TrackRulesV01 &info, Lane &lane );
  void AdjustFrozenOrderOld( TrackRulesV01 &info );
#if 1 // choose one
  void AdjustFrozenOrder( TrackRulesV01 &info ) { for( long i = 0; i < TRCOL_MAX_LANES; ++i ) AdjustFrozenOrderNew( info, mLane[ i ] ); }
#else
  void AdjustFrozenOrder( TrackRulesV01 &info ) { AdjustFrozenOrderOld( info ); }
#endif
  void RemoveFromOrder( const KnownCar *knownCar );

        KnownCar *GetKnownCarByID( long id );
  const KnownCar *GetKnownCarByID( long id ) const;

  void ClearAllPassStatuses();
  void ClearPassStatuses( const KnownCar *knownCar );
  PassStatus GetPassStatus( const KnownCar *passer, const KnownCar *passee ) const;
  void SetPassStatus( const KnownCar *passer, const KnownCar *passee, PassStatus v );

 public:

  //
  StockCarRulesPlugin();
  ~StockCarRulesPlugin();

  //
  void StartSession();
  void SetEnvironment( const EnvironmentInfoV01 &info ); // may be called whenever the environment changes

  //
  bool WantsScoringUpdates()                                { return( true ); } // whether we want scoring updates
  void UpdateScoring( const ScoringInfoV01 &info );                             // update plugin with scoring info (approximately five times per second)

  //
  bool WantsTrackRulesAccess()                              { return( true ); } // change to true in order to read or write track order (during formation or caution laps)
  bool AccessTrackRules( TrackRulesV01 &info );                                 // current track order passed in; return true if you want to change it (note: this will be called immediately after UpdateScoring() when appropriate)

  //
  bool GetCustomVariable( long i, CustomVariableV01 &var );
  void AccessCustomVariable( CustomVariableV01 &var );
  void GetCustomVariableSetting( CustomVariableV01 &var, long i, CustomSettingV01 &setting );
};


#endif // _STOCK_CAR_RULES_HPP_


