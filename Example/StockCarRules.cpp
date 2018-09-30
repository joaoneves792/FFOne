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

#include "StockCarRules.hpp"
#include <cstring>
#include <cfloat>


// plugin information

extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "Stock Car Rules - 2015.04.28" ); }

extern "C" __declspec( dllexport )
PluginObjectType __cdecl GetPluginType()               { return( PO_INTERNALS ); }

extern "C" __declspec( dllexport )
int __cdecl GetPluginVersion()                         { return( 7 ); }  // InternalsPluginV07 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec( dllexport )
PluginObject * __cdecl CreatePluginObject()            { return( static_cast< PluginObject * >( new StockCarRulesPlugin ) ); }

extern "C" __declspec( dllexport )
void __cdecl DestroyPluginObject( PluginObject *obj )  { delete( static_cast< StockCarRulesPlugin * >( obj ) ); }


// StockCarRules class

StockCarRulesPlugin::StockCarRulesPlugin()
{
  mLogging = false;
  mLogPath = NULL;
  mLogFile = NULL;

  mAdjustFrozenOrder = 0.25f; // Similar to old PLR option "Adjust Frozen Order"
  mAllowFrozenAdjustments = 1;
  mAdjustUntilYellowFlagState = 4;
  mEnableLuckyDogFreePass = true;

  mKnownCar = NULL;
  mFrozenPass = NULL;

  // mLane[].mMember already nulled out
  mAction = NULL;
  Clear();
}


StockCarRulesPlugin::~StockCarRulesPlugin()
{
  // logging cleanup
  mLogging = false;

  delete [] mLogPath;
  mLogPath = NULL;

  if( mLogFile != NULL )
  {
    fclose( mLogFile );
    mLogFile = NULL;
  }

  // other data cleanup
  Clear();
}


void StockCarRulesPlugin::StartSession()
{
  // open log file if desired (and not already open)
  if( mLogging && ( mLogFile == NULL ) && ( mLogPath != NULL ) )
  {
    const size_t pathLen = strlen( mLogPath );
    if( pathLen > 0 )
    {
      const size_t fullLen = pathLen + 50;
      char *fullpath = new char[ fullLen ];
      long count = 0;
      while( true )
      {
        strcpy_s( fullpath, fullLen, mLogPath );
        if( fullpath[ pathLen - 1 ] != '\\' )
          strcat_s( fullpath, fullLen, "\\" );
        strcat_s( fullpath, fullLen, "Log\\StockCarRulesPluginLog" );

        // tag.2016.02.26 - people never remember to save this file off, so just keep creating new ones ...
        char numb[ 64 ];
        sprintf_s( numb, "%d.txt", count++ );
        strcat_s( fullpath, fullLen, numb );

        fopen_s( &mLogFile, fullpath, "r" );
        if( mLogFile == NULL )
        {
          fopen_s( &mLogFile, fullpath, "w" );
          break;
        }
        fclose( mLogFile );
        mLogFile = NULL;
      }

      delete [] fullpath;
    }
  }

  if( mLogFile != NULL )
    fprintf( mLogFile, "\n--- NEW SESSION STARTING ---\n" );

  // clear all data (except logging)
  Clear();
}


void StockCarRulesPlugin::SetEnvironment( const EnvironmentInfoV01 &info )
{
  // If we already have it, but it's wrong, delete it.
  if( mLogPath != NULL )
  {
    if( 0 != _stricmp( mLogPath, info.mPath[ 0 ] ) )
    {
      delete [] mLogPath;
      mLogPath = NULL;
    }
  }

  // If we don't already have it now, then copy it.
  if( mLogPath == NULL )
  {
    const size_t pathLen = strlen( info.mPath[ 0 ] ) + 1;
    mLogPath = new char[ pathLen ];
    strcpy_s( mLogPath, pathLen, info.mPath[ 0 ] );
  }
}


void StockCarRulesPlugin::Clear()
{
  mMaxKnownCars = 0;
  mNumKnownCars = 0;
  delete [] mKnownCar;
  mKnownCar = NULL;

  delete [] mFrozenPass;
  mFrozenPass = NULL;

  for( long i = 0; i < TRCOL_MAX_LANES; ++i )
    mLane[ i ].mNumMembers = 0;

  mMaxActions = 0;
  mNumActions = 0;
  delete [] mAction;
  mAction = NULL;

  mProcessedLastLapOfCaution = false;
  mLuckyDogID = -1;
  mLeaderLaneChoiceLateral = DBL_MAX;
  mYellowFlagLaps = 0;
  mCautionET = -1.0;
  mRandomGreenDist = -1.0f;

  mSession = -1;
  mCurrentET = -1.0;
  mEndET = -1.0;
  mMaxLaps = -1;
  mFullLapDist = -1.0;
  mGamePhase = static_cast< unsigned char >( -1 );
  mYellowFlagState = -1;
}


void StockCarRulesPlugin::AddMember( TrackRulesColumnV01 colIndex, const Lane::Member &member )
{
  if( mLogFile != NULL )
    fprintf( mLogFile, "Adding ID=%d tld=%.1f to lane %d et=%.3f\n", member.mID, member.mDist, colIndex, mCurrentET );
  mLane[ colIndex ].AddMember( member );
}


void StockCarRulesPlugin::UpdateScoring( const ScoringInfoV01 &info )
{
  // No rules outside of the race session for now
  if( info.mSession < 10 ) // 10-13 are race session
    return;

  // Copy any basic scoring info desired
  mSession = info.mSession;
  mCurrentET = info.mCurrentET;
  mEndET = info.mEndET;
  mMaxLaps = info.mMaxLaps;
  mFullLapDist = info.mLapDist;
  mGamePhase = info.mGamePhase;
  mYellowFlagState = info.mYellowFlagState;

  // Analyze vehicles
  // UNDONE - Remove known cars that are no longer in this list or being scored? (Would probably need to be done after the loop.)
  for( long i = 0; i < info.mNumVehicles; ++i )
  {
    // Look at vehicles currently being scored
    const VehicleScoringInfoV01 &vsi = info.mVehicle[ i ];
    if( !vsi.mServerScored )
      continue;

    // Add our own local information about this vehicle if this is the first we've heard of it
    long knownCarCount;
    for( knownCarCount = 0; knownCarCount < mNumKnownCars; ++knownCarCount )
    {
      if( vsi.mID == mKnownCar[ knownCarCount ].mID )
        break;
    }

    if( knownCarCount == mNumKnownCars )
    {
      if( mNumKnownCars >= mMaxKnownCars )
      {
        KnownCar *newKnownCar = new KnownCar[ ++mMaxKnownCars ];
        for( long j = 0; j < mNumKnownCars; ++j )
          newKnownCar[ j ] = mKnownCar[ j ];
        delete [] mKnownCar;
        mKnownCar = newKnownCar;
      }

      KnownCar &knownCar = mKnownCar[ mNumKnownCars ];
      knownCar.mIndex = mNumKnownCars++;
      knownCar.mID = vsi.mID;
      knownCar.mLapsDown = 0;
      knownCar.mWorstYellowSeverity = 0.0f;
      knownCar.mPittedAtDesignatedTime = false;
      knownCar.mPittedAtSomeOtherTime = false;

      // We need these initialized in case we are detecting changes (mInPits, for example), even though this
      // is the same code as below the conditional.
      knownCar.mUnderYellow = vsi.mUnderYellow;
      knownCar.mInPits = vsi.mInPits;
      knownCar.mPitState = vsi.mPitState;
      knownCar.mCountLapFlag = vsi.mCountLapFlag;
      knownCar.mFinishStatus = vsi.mFinishStatus;
      knownCar.mPlace = vsi.mPlace;
      knownCar.mTotalLaps = vsi.mTotalLaps;
      knownCar.mNumEOLLs = 0;

      strcpy_s( knownCar.mDriverName, vsi.mDriverName );
      knownCar.mPathLateral = vsi.mPathLateral;
      knownCar.mLapDist = vsi.mLapDist;

      if( mLogFile != NULL )
        fprintf( mLogFile, "New ID %d \"%s\" on list of known cars\n", knownCar.mID, vsi.mDriverName );

      // increase size of pass status array
      PassStatus *newFrozenPass = new PassStatus[ mNumKnownCars * mNumKnownCars ];
      for( long x = 0; x < mNumKnownCars; ++x )
      {
        for( long y = 0; y < mNumKnownCars; ++y )
        {
          if( ( x < ( mNumKnownCars - 1 ) ) && ( y < ( mNumKnownCars - 1 ) ) )
            newFrozenPass[ ( x * mNumKnownCars ) + y ] = mFrozenPass[ ( x * ( mNumKnownCars - 1 ) ) + y ];
          else
            newFrozenPass[ ( x * mNumKnownCars ) + y ] = PS_NONE;
        }
      }
      delete [] mFrozenPass;
      mFrozenPass = newFrozenPass;
    }

    // OK, now update the information about this known car.
    KnownCar &knownCar = mKnownCar[ knownCarCount ];
    knownCar.mUnderYellow = vsi.mUnderYellow;
    if( knownCar.mInPits != vsi.mInPits )
    {
      knownCar.mInPits = vsi.mInPits;
      if( knownCar.mInPits )
      {
        // Looks like we entered pits ... detect whether this was at the appropriate time
        if( 0 == knownCar.mLapsDown )
        {
          if( info.mYellowFlagState == 3 ) // 3 means pits are open for lead lap vehicles
          {
            knownCar.mPittedAtDesignatedTime = true;
          }
          else
          {
            // UNDONE - this may actually be a penalty in some cases
            knownCar.mPittedAtSomeOtherTime = true;
          }
        }
        else
        {
          if( info.mYellowFlagState == 4 ) // 4 means pits are open for basically everybody, I think (or is it supposed to be lapped vehicles?)
          {
            knownCar.mPittedAtDesignatedTime = true;
          }
          else
          {
            // UNDONE - this may actually be a penalty in some cases
            knownCar.mPittedAtSomeOtherTime = true;
          }
        }

        // While we're here, remove them from the track order if they were in it
        RemoveFromOrder( &knownCar );
      }
    }
    knownCar.mPitState = vsi.mPitState;
    knownCar.mCountLapFlag = vsi.mCountLapFlag;
    knownCar.mFinishStatus = vsi.mFinishStatus;
    knownCar.mPlace = vsi.mPlace;
    knownCar.mTotalLaps = vsi.mTotalLaps;
    strcpy_s( knownCar.mDriverName, vsi.mDriverName );
    knownCar.mPathLateral = vsi.mPathLateral;
    knownCar.mLapDist = vsi.mLapDist;
  }
}


bool StockCarRulesPlugin::SafetyCarIsApproachingPitlane( TrackRulesV01 &info, long numLaps )
{
  // Give a reasonable distance to open pitlane
  const float pitlaneApproachDist = info.mPitLaneStartDist - 100.0f;
  const float trackLen = static_cast< float >( mFullLapDist );

  // Get safety car lap distance and wraparound to detect during the short section
  // beyond the entrance (30 meters).
  const float detectionThreshold = 30.0f;
  float safetyCarLapDist = static_cast< float >( info.mSafetyCarLapDist );
  if( safetyCarLapDist > ( pitlaneApproachDist + detectionThreshold ) )
    safetyCarLapDist -= trackLen;
  else if( safetyCarLapDist < ( pitlaneApproachDist - ( trackLen - detectionThreshold ) ) )
    safetyCarLapDist += trackLen;

  // Get safety car starting distance and wraparound to be comparable to pitlane approach
  float safetyCarStartDist = info.mSafetyCarLapDistAtStart;
  if( safetyCarStartDist > ( pitlaneApproachDist + detectionThreshold ) )
    safetyCarStartDist -= trackLen;
  else if( safetyCarStartDist < ( pitlaneApproachDist - ( trackLen - detectionThreshold ) ) )
    safetyCarStartDist += trackLen;

  // Create a distance that tries to ensure all cars finish a lap before the pits open
  float tooClose = 200.0f;
  const float maxClose = 0.60f * trackLen;
  if( tooClose > maxClose )
    tooClose = maxClose;

  // If safety car starts too close to pitlane entrance, then force another formation lap
  if( safetyCarStartDist > ( pitlaneApproachDist - tooClose ) )
    ++numLaps;

  // Finally check if safety car is beyond and the required formation laps have been completed
  if( ( safetyCarLapDist > pitlaneApproachDist ) && ( info.mSafetyCarLaps >= numLaps ) )
    return( true );
  else
    return( false );
}


StockCarRulesPlugin::KnownCar *StockCarRulesPlugin::GetKnownCarByID( long id )
{
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    KnownCar *knownCar = &mKnownCar[ i ];
    if( knownCar->mID == id )
      return( knownCar );
  }

  if( mLogFile != NULL )
  {
    fprintf( mLogFile, "ERROR: Looking for ID %d but couldn't find it\n" );
    fflush( mLogFile );
  }

  return( NULL );
}


const StockCarRulesPlugin::KnownCar *StockCarRulesPlugin::GetKnownCarByID( long id ) const
{
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    const KnownCar *knownCar = &mKnownCar[ i ];
    if( knownCar->mID == id )
      return( knownCar );
  }

  if( mLogFile != NULL )
  {
    fprintf( mLogFile, "ERROR: Looking for ID %d but couldn't find it\n" );
    fflush( mLogFile );
  }

  return( NULL );
}


void StockCarRulesPlugin::ApplyLuckyDogFreePass( TrackRulesV01 &info )
{
  // tag.2016.05.23 - See if this feature is disabled.
  if( !mEnableLuckyDogFreePass )
  {
    if( mLogFile != NULL )
      fprintf( mLogFile, "Lucky dog/free pass feature is disabled\n" );
    mLuckyDogID = 9999; // big number that indicates it has been "applied", but not to a valid car
    return;
  }

  // Make sure it's not given to somebody who caused yellow
  // UNDONE - Or nudged someone into spinning!?
  // UNDONE - Should mAdjustFrozenOrder be included in this threshold?
  const float threshold = 0.15f * info.mSafetyCarThreshold;

  // Look for highest-placed vehicle that is at least a lap down
  bool causedYellow = false;
  long luckyPassIndex = -1;
  long luckyPassLapsDown = LONG_MAX;
  Lane &rightLane = mLane[ TRCOL_RIGHT_LANE ];
  for( long j = 0; j < rightLane.mNumMembers; ++j )
  {
    const KnownCar *knownCar = GetKnownCarByID( rightLane.mMember[ j ].mID );
    if( ( knownCar != NULL ) && ( knownCar->mLapsDown > 0 ) && ( luckyPassLapsDown > knownCar->mLapsDown ) )
    {
      if( mLogFile != NULL )
        fprintf( mLogFile, "Considering ID %d for lucky dog/free pass, severity=%.2f lapsdown=%d\n", knownCar->mID, knownCar->mWorstYellowSeverity, knownCar->mLapsDown );

      luckyPassIndex = j;
      luckyPassLapsDown = knownCar->mLapsDown;
      causedYellow = ( knownCar->mWorstYellowSeverity > threshold );
    }
  }

  if( luckyPassIndex >= 0 )
  {
    // Grab the ID (we could've done this above, but no matter)
    const long id = rightLane.mMember[ luckyPassIndex ].mID;

    // If candidate caused yellow, then nobody gets it
    if( causedYellow )
    {
      if( mLogFile != NULL )
        fprintf( mLogFile, "Lucky dog candidate was index %d ID %d, but they caused yellow so nobody gets it!\n", luckyPassIndex, id );
      mLuckyDogID = 9999; // big number that indicates it has been "applied", but not to a valid car
    }
    else
    {
      if( mLogFile != NULL )
        fprintf( mLogFile, "Chose index %d ID %d for lucky dog\n", luckyPassIndex, id );

      // Scoot 'em down to the back of the lane ...
      for( long n = luckyPassIndex; n < ( rightLane.mNumMembers - 1 ); ++n ) // swaps repeatedly until last
      {
        const Lane::Member swap = rightLane.mMember[ n ];
        rightLane.mMember[ n ] = rightLane.mMember[ n + 1 ];
        rightLane.mMember[ n + 1 ] = swap;
      }

      // ... but up a lap!
      for( long j = 0; j < info.mNumParticipants; ++j )
      {
        TrackRulesParticipantV01 &trp = info.mParticipant[ j ];
        if( trp.mID == id )
        {
          --trp.mRelativeLaps;
          strcpy_s( trp.mMessage, "Lucky Dog: Pass Field On Left" );
        }
        else
        {
          strcpy_s( trp.mMessage, "Allow Lucky Dog To Pass On Left" );
        }
      }

      // Tell everybody who the lucky dog is ... unfortunately this can't be easily translated
      // since we are transmitting the whole phrase in multiplayer, and the whole phrase includes
      // the driver name ...
      const KnownCar *knownCar = GetKnownCarByID( id );
      if( knownCar != NULL )
        sprintf_s( info.mMessage, "Lucky Dog: %s", knownCar->mDriverName );

      // And set ID (which incidentally tells us that it has been applied)
      mLuckyDogID = id;
    }
  }
}


const StockCarRulesPlugin::KnownCar *StockCarRulesPlugin::GetFirstCarInRightLaneThatIsOnTheLeadLap() const
{
  // First member in right lane that is not a lap down is the leader
  for( long i = 0; i < mLane[ TRCOL_RIGHT_LANE ].mNumMembers; ++i )
  {
    const KnownCar *knownCar = GetKnownCarByID( mLane[ TRCOL_RIGHT_LANE ].mMember[ i ].mID );
    if( ( knownCar != NULL ) && ( 0 == knownCar->mLapsDown ) )
      return( knownCar );
  }

  return( NULL );
}


struct SortableLaneMember
{
  long mID;                             // slot ID
  double mDist;                         // distance around track
};


int _cdecl StockCarRulesPlugin::CompareFrozen( const void *slot1, const void *slot2 )
{
  const SortableLaneMember * const one = reinterpret_cast< const SortableLaneMember * >( slot1 );
  const SortableLaneMember * const two = reinterpret_cast< const SortableLaneMember * >( slot2 );

  if(      one->mDist < two->mDist ) return(  1 );
  else if( one->mDist > two->mDist ) return( -1 );
  else                               return(  0 );
}


void StockCarRulesPlugin::FreezeOrder( TrackRulesV01 &info )
{
  // Log
  if( mLogFile != NULL )
    fprintf( mLogFile, "FreezeOrder()\n" );

  // Create order around the track.
  long leaderID = -1;
  double leaderTotalDist = -FLT_MAX;
  SortableLaneMember *tempLane = new SortableLaneMember[ mNumKnownCars ];
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    const KnownCar &knownCar = mKnownCar[ i ];
    tempLane[ i ].mID = knownCar.mID;
    tempLane[ i ].mDist = knownCar.mLapDist;
    if( mLogFile != NULL )
      fprintf( mLogFile, "id=%d place=%2d totallaps=%3d lapdist=%6.1f dist=%6.1f\n", knownCar.mID, knownCar.mPlace, knownCar.mTotalLaps, knownCar.mLapDist, tempLane[ i ].mDist );

    // NOTE: Right now we're adding cars even if they're in the pits, which will get cleaned up during maintenance. But I'm
    //       not sure if that will result in the right car being picked up by the safety car in all cases. For example, if the
    //       leader is in the pits and there are cars between the leader and 2nd-place, does the safety car pick up the cars
    //       in between, or does it pick up 2nd-place (which is the highest place car currently on track)?
    const double totalDist = knownCar.CorrectedTotalLapDist( mFullLapDist );
//    if( ( totalDist > leaderTotalDist ) && !knownCar.mInPits ) // use this to pick up highest-place car on track
    if( totalDist > leaderTotalDist ) // use this to pick up highest-place car, regardless of whether it is on track or in pits
    {
      leaderID = knownCar.mID;
      leaderTotalDist = totalDist;
    }
  }

  qsort( tempLane, mNumKnownCars, sizeof( SortableLaneMember ), CompareFrozen );

  // Find the leader in this order (hopefully that won't fail, but as a backup plan we'll try the 2nd-place car, 3rd-place, etc.)
  long leaderIndex = mNumKnownCars;
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    if( tempLane[ i ].mID == leaderID )
    {
      leaderIndex = i;
      break;
    }
  }

  if( leaderIndex >= mNumKnownCars ) // We really couldn't find anybody!??! Then use arbitrary choice.
    leaderIndex = 0;

  // Calculate the number of laps down
  bool everybodyWrapped = false;
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    KnownCar &knownCar = mKnownCar[ i ];
    const double currentTotalLapDist = knownCar.CorrectedTotalLapDist( mFullLapDist );
    knownCar.mLapsDown = static_cast< long >( ( leaderTotalDist - currentTotalLapDist ) / mFullLapDist );

    // Initialize some other info
    knownCar.mPittedAtDesignatedTime = false;
    knownCar.mPittedAtSomeOtherTime = false;

    // Special case: If the leader hasn't crossed s/f line for the first time on a rolling start, we need to adjust all relative laps
    //               because it won't get incremented at the s/f line (see KnownCar::CorrectedTotalLapDist() for similar logic).
    if( knownCar.mID == leaderID )
    {
      if( ( 0 == knownCar.mTotalLaps ) && ( 0 == knownCar.mCountLapFlag ) )
        everybodyWrapped = true;
    }
  }

  if( mLogFile != NULL )
    fprintf( mLogFile, "leaderID=%d leaderIndex=%d\n", leaderID, leaderIndex );

  // Clear all lanes
  for( long i = 0; i < TRCOL_MAX_LANES; ++i )
    mLane[ i ].mNumMembers = 0;

  // Fill in frozen order starting with leader.
  const long numFrozen = mNumKnownCars;
  for( long k = 0; k < numFrozen; ++k )
  {
    long tempIndex = ( k + leaderIndex );
    const bool wrappedAround = ( tempIndex >= numFrozen );
    if( wrappedAround )
      tempIndex -= numFrozen;

    // Initialize a member to add some lane
    Lane::Member laneMember;
    laneMember.mID = tempLane[ tempIndex ].mID;

    if( mLogFile != NULL )
      fprintf( mLogFile, "tempIndex=%2d id=%2d wa=%d ", tempIndex, laneMember.mID, wrappedAround );

    // Find corresponding car
    const KnownCar *knownCar = GetKnownCarByID( laneMember.mID );
    if( knownCar != NULL )
    {
      if( mLogFile != NULL )
        fprintf( mLogFile, "frozen=%2d place=%2d totallaps=%3d lapdist=%6.1f ip=%d fs=%d\n", k, knownCar->mPlace, knownCar->mTotalLaps, knownCar->mLapDist, knownCar->mInPits, knownCar->mFinishStatus );

      laneMember.mDist = knownCar->CorrectedTotalLapDist( mFullLapDist );
      AddMember( TRCOL_RIGHT_LANE, laneMember ); // putting them in the right (outside) lane

      // Cars with a higher wraparound lap distance than the leader need their relative laps adjusted (so that
      // they cross the finish line before the leader, and catch up and stay on the same caution lap):
      if( wrappedAround || everybodyWrapped )
      {
        for( long n = 0; n < info.mNumParticipants; ++n )
        {
          TrackRulesParticipantV01 &trp = info.mParticipant[ n ];
          if( trp.mID == laneMember.mID )
            --trp.mRelativeLaps;
        }
      }
    }
  }

  // Deallocate
  delete [] tempLane;
  tempLane = NULL;

  // *Now* go back and remove cars from the track order if in pits. We it this way (rather than only adding those on
  // the track in the first place) to be consistent with how it was originally done in rF. It might even be simpler.
  // tag.2015.08.22 - Added a check for being DNF'd or DQ'd, because that could happen without being in the pits.
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    const KnownCar &knownCar = mKnownCar[ i ];
    if( knownCar.mInPits || ( knownCar.mFinishStatus >= 2 ) ) // mFinishStatus will be: 0=none, 1=finished, 2=dnf, 3=dq
      RemoveFromOrder( &knownCar );
  }

  // Log clarification (after removals):
  if( mLogFile != NULL )
  {
    fprintf( mLogFile, "Starting order on track:\n" );
    for( long i = 0; i < mLane[ TRCOL_RIGHT_LANE ].mNumMembers; ++i )
    {
      const KnownCar *knownCar = GetKnownCarByID( mLane[ TRCOL_RIGHT_LANE ].mMember[ i ].mID );
      if( knownCar != NULL )
        fprintf( mLogFile, "%3d: id=%d \"%s\"\n", i, knownCar->mID, knownCar->mDriverName );
    }
  }

  // Keep track of pass statuses, starting with none:
  ClearAllPassStatuses();
}


void StockCarRulesPlugin::ClearAllPassStatuses()
{
  if( mLogFile != NULL )
    fprintf( mLogFile, "ClearAllPassStatuses()\n" );
  if( mFrozenPass != NULL )
  {
    for( long i = 0; i < ( mNumKnownCars * mNumKnownCars ); ++i )
      mFrozenPass[ i ] = PS_NONE;
  }
}


void StockCarRulesPlugin::ClearPassStatuses( const KnownCar *knownCar )
{
  if( mLogFile != NULL )
    fprintf( mLogFile, "ClearPassStatuses( \"%s\" ) et=%.3f\n", knownCar->mDriverName, mCurrentET );

  if( mFrozenPass != NULL )
  {
    for( long i = 0; i < mNumKnownCars; ++i )
    {
      mFrozenPass[ ( i * mNumKnownCars ) + knownCar->mIndex ] = PS_NONE;
      mFrozenPass[ ( knownCar->mIndex * mNumKnownCars ) + i ] = PS_NONE;
    }
  }
}


StockCarRulesPlugin::PassStatus StockCarRulesPlugin::GetPassStatus( const KnownCar *passer, const KnownCar *passee ) const
{
  if( ( passer->mIndex > mNumKnownCars ) || ( passee->mIndex >= mNumKnownCars ) )
  {
    if( mLogFile != NULL )
      fprintf( mLogFile, "ERROR in GetPassStatus(): %d %d %d\n", passer->mIndex, passee->mIndex, mNumKnownCars );
    return( PS_NONE );
  }

  const long matrixIndex = ( passer->mIndex * mNumKnownCars ) + passee->mIndex;
  return( mFrozenPass[ matrixIndex ] );
}


void StockCarRulesPlugin::SetPassStatus( const KnownCar *passer, const KnownCar *passee, PassStatus v )
{
  if( ( passer->mIndex > mNumKnownCars ) || ( passee->mIndex >= mNumKnownCars ) )
  {
    if( mLogFile != NULL )
      fprintf( mLogFile, "ERROR in SetPassStatus(): %d %d %d\n", passer->mIndex, passee->mIndex, mNumKnownCars );
    return;
  }

  const PassStatus oppPassStatus = mFrozenPass[ ( passee->mIndex * mNumKnownCars ) + passer->mIndex ];
  if( ( PS_NONE != v ) && ( PS_NONE != oppPassStatus ) )
  {
    if( mLogFile != NULL )
    {
      fprintf( mLogFile, "ERROR in SetPassStatus() - both vehicles appear to have passed each other \"%s\" %d \"%s\" %d\n",
               passer->mDriverName, v, passee->mDriverName, oppPassStatus );
    }
  }

  const long matrixIndex = ( passer->mIndex * mNumKnownCars ) + passee->mIndex;
  if( mLogFile != NULL )
    fprintf( mLogFile, "Changing status from %d to %d for \"%s\" passing \"%s\" et=%.3f\n", mFrozenPass[ matrixIndex ], v, passer->mDriverName, passee->mDriverName, mCurrentET );
  mFrozenPass[ matrixIndex ] = v;
}


void StockCarRulesPlugin::AdjustFrozenOrderNew( TrackRulesV01 &info, Lane &lane )
{
  // Note: I'm not sure if we'll need this in lanes other than the right, but we allow it in this new version of this function.

  // Threshold to move people down the frozen order because they are crashed or spun or something.
  const float normalThreshold = mAdjustFrozenOrder * info.mSafetyCarThreshold;
  const float threshold = ( 0 == mAllowFrozenAdjustments ) ? FLT_MAX : ( 1 == mAllowFrozenAdjustments ) ? normalThreshold : -1.0f;

#if 1
  // First check for any passes between vehicles in this lane and make a note of it in our FrozenPass matrix
  for( long k = 0; k < lane.mNumMembers; ++k )
  {
    const KnownCar *knownCar = GetKnownCarByID( lane.mMember[ k ].mID );
    const double prevTotalDist = lane.mMember[ k ].mDist;
    const double curTotalDist = knownCar->CorrectedTotalLapDist( mFullLapDist );

    // Check against all members that are supposed to be behind this car (this avoids repeating a check)
    for( long j = k + 1; j < lane.mNumMembers; ++j )
    {
      const KnownCar *oppKnownCar = GetKnownCarByID( lane.mMember[ j ].mID );
      const double oppPrevTotalDist = lane.mMember[ j ].mDist;
      const double oppCurTotalDist = oppKnownCar->CorrectedTotalLapDist( mFullLapDist );

      // Have we gone another lap down?
      const long curLapsDown = static_cast< long >( floor( ( oppCurTotalDist - curTotalDist ) / mFullLapDist ) );
      const long origLapsDown = static_cast< long >( floor( ( oppPrevTotalDist - prevTotalDist ) / mFullLapDist ) );
      if( ( mLogFile != NULL ) && ( curLapsDown != origLapsDown ) )
        fprintf( mLogFile, "\"%s\" vs. \"%s\": lapsdown=(%d->%d)\n", knownCar->mDriverName, oppKnownCar->mDriverName, origLapsDown, curLapsDown );

      if(      curLapsDown > origLapsDown ) // oppKnownCar passed knownCar, update relevant statuses
      {
        if( PS_NONE == GetPassStatus( knownCar, oppKnownCar ) )
        {
          // check that we didn't already record this pass legally ...
          if( PS_LEGAL == GetPassStatus( oppKnownCar, knownCar ) )
          {
            if( mLogFile != NULL )
              fprintf( mLogFile, "Error: \"%s\" already made legal pass on \"%s\"\n", oppKnownCar->mDriverName, knownCar->mDriverName );
          }
          else
          {
            SetPassStatus( oppKnownCar, knownCar, PS_PHYSICAL );
          }
        }
        else
        {
          // previously recorded the opposite pass, so clear them both out now
          SetPassStatus( knownCar, oppKnownCar, PS_NONE );
          SetPassStatus( oppKnownCar, knownCar, PS_NONE );
        }
      }
      else if( curLapsDown < origLapsDown ) // knownCar passed oppKnownCar, update relevant statuses
      {
        if( PS_NONE == GetPassStatus( oppKnownCar, knownCar ) )
        {
          // check that we didn't already record this pass legally ...
          if( PS_LEGAL == GetPassStatus( knownCar, oppKnownCar ) )
          {
            if( mLogFile != NULL )
              fprintf( mLogFile, "Error: \"%s\" already made legal pass on \"%s\"\n", knownCar->mDriverName, oppKnownCar->mDriverName );
          }
          else
          {
            SetPassStatus( knownCar, oppKnownCar, PS_PHYSICAL );
          }
        }
        else
        {
          // previously recorded the opposite pass, so clear them both out now
          SetPassStatus( oppKnownCar, knownCar, PS_NONE );
          SetPassStatus( knownCar, oppKnownCar, PS_NONE );
        }
      }
    }
  }

  // Second, see if any cars are causing a yellow, and make all physical passes against that car into legal passes
  bool firstConversion = true;
  for( long k = 0; k < lane.mNumMembers; ++k )
  {
    const KnownCar *knownCar = GetKnownCarByID( lane.mMember[ k ].mID );

    // Find participant (to get current yellow severity value)
    for( long i = 0; i < info.mNumParticipants; ++i )
    {
      TrackRulesParticipantV01 &trp = info.mParticipant[ i ];
      if( trp.mID != knownCar->mID )
        continue;

      // Is it compromised in some way (yellow 'severity' exceeds threshold)?
      if( trp.mYellowSeverity > threshold )
      {
        for( long j = 0; j < lane.mNumMembers; ++j )
        {
          if( j == k )
            continue;

          // Change all physical passes against this car into legal passes
          const KnownCar *oppKnownCar = GetKnownCarByID( lane.mMember[ j ].mID );
          if( PS_PHYSICAL == GetPassStatus( oppKnownCar, knownCar ) )
          {
            if( firstConversion )
            {
              firstConversion = false;
              if( mLogFile != NULL )
                fprintf( mLogFile, "Converting physical passes to legal passes:\n" );
            }
            SetPassStatus( oppKnownCar, knownCar, PS_LEGAL );
          }
        }
      }
    }
  }

  // Now adjust order for any legal passes
  bool stillAdjustingOrder = true;
  while( stillAdjustingOrder )
  {
    stillAdjustingOrder = false;

    for( long k = 0; k < lane.mNumMembers; ++k )
    {
      KnownCar *knownCar = GetKnownCarByID( lane.mMember[ k ].mID );

      const long j = ( k + 1 ) % lane.mNumMembers;
      KnownCar *oppKnownCar = GetKnownCarByID( lane.mMember[ j ].mID );

      if( PS_LEGAL == GetPassStatus( oppKnownCar, knownCar ) )
      {
        // This keeps the loop running, because one car may need to get swapped multiple times
        stillAdjustingOrder = true;

        // Swap vehicles (there are a couple special cases here if one of the cars is the leader):
        if( ( k == 0 ) && ( oppKnownCar->mLapsDown > 0 ) )
        {
          if( mLogFile != NULL )
            fprintf( mLogFile, "Special case #1: Swapping \"%s\" around leader \"%s\" and therefore they gain lap\n", oppKnownCar->mDriverName, knownCar->mDriverName );

          // Special case: If leader causes yellow and gets passed by someone a lap or more down, that someone gets to
          // continue around and join at the end of the line.
          // assert( j == 1 );
          for( long n = j; n < ( lane.mNumMembers - 1 ); ++n ) // 2nd guy swaps repeatedly until last
          {
            const Lane::Member swap = lane.mMember[ n ];
            lane.mMember[ n ] = lane.mMember[ n + 1 ];
            lane.mMember[ n + 1 ] = swap;
          }

          // Fix guy's lap down data
          --oppKnownCar->mLapsDown;
          for( long n = 0; n < info.mNumParticipants; ++n )
          {
            TrackRulesParticipantV01 &oppTrp = info.mParticipant[ n ];
            if( oppKnownCar->mID == oppTrp.mID )
            {
              --oppTrp.mRelativeLaps;
              break;
            }
          }
        }
        else if( k == ( lane.mNumMembers - 1 ) )
        {
          if( mLogFile != NULL )
            fprintf( mLogFile, "Special case #2: Swapping \"%s\" behind leader \"%s\" and therefore they lose lap\n", knownCar->mDriverName, oppKnownCar->mDriverName );

          // Special case: Last place fella must've been passed by leader causing yellow ... move his frozen order
          // location behind the leader and a(nother?) lap down.
          // assert( j == 0 );
          for( long n = k - 1; n > j; --n ) // last guy swap repeatedly until 2nd
          {
            const Lane::Member swap = lane.mMember[ n ];
            lane.mMember[ n ] = lane.mMember[ n + 1 ];
            lane.mMember[ n + 1 ] = swap;
          }

          // Fix guy's lap down data
          ++knownCar->mLapsDown;
          for( long n = 0; n < info.mNumParticipants; ++n )
          {
            TrackRulesParticipantV01 &trp = info.mParticipant[ n ];
            if( knownCar->mID == trp.mID )
            {
              ++trp.mRelativeLaps;
              break;
            }
          }
        }
        else
        {
          if( mLogFile != NULL )
            fprintf( mLogFile, "Swapping \"%s\" ahead of \"%s\"\n", oppKnownCar->mDriverName, knownCar->mDriverName );

          // Normal swap!
          // assert( j == ( k + 1 ) );
          const Lane::Member swap = lane.mMember[ j ];
          lane.mMember[ j ] = lane.mMember[ k ];
          lane.mMember[ k ] = swap;
        }

        // Clear the legal pass we just processed
        SetPassStatus( oppKnownCar, knownCar, PS_NONE );
      }
    }
  }

  // UNDONE
#endif

  // Update all lap distances which will be the "previous" distance next time.
  for( long k = 0; k < lane.mNumMembers; ++k )
  {
    const KnownCar *knownCar = GetKnownCarByID( lane.mMember[ k ].mID );
    if( knownCar != NULL )
      lane.mMember[ k ].mDist = knownCar->CorrectedTotalLapDist( mFullLapDist );
  }
}


void StockCarRulesPlugin::AdjustFrozenOrderOld( TrackRulesV01 &info )
{
  // NOTE: this function assumes everybody is in the right lane, which is where FreezeOrder() originally put them.
  Lane &rightLane = mLane[ TRCOL_RIGHT_LANE ];

  // Threshold to move people down the frozen order because they are crashed or spun or something.
  const float normalThreshold = mAdjustFrozenOrder * info.mSafetyCarThreshold;
  const float threshold = ( 0 == mAllowFrozenAdjustments ) ? FLT_MAX : ( 1 == mAllowFrozenAdjustments ) ? normalThreshold : -1.0f;

  // Look through the participants
  for( long i = 0; i < info.mNumParticipants; ++i )
  {
    // Is it compromised in some way (yellow 'severity' exceeds threshold)?
    TrackRulesParticipantV01 &trp = info.mParticipant[ i ];
    if( trp.mYellowSeverity > threshold )
    {
      KnownCar *knownCar = GetKnownCarByID( trp.mID );
      if( knownCar != NULL )
      {
        // If this car has already crossed s/f line under yellow, don't bother demoting it in the frozen order
        if( knownCar->mUnderYellow )
          continue;

        // Find this compromised vehicle in the current frozen order.
        for( long k = 0; k < rightLane.mNumMembers; ++k )
        {
          if( rightLane.mMember[ k ].mID != trp.mID )
            continue;

          if( ( 1 == mAllowFrozenAdjustments ) && ( mLogFile != NULL ) )
            fprintf( mLogFile, "Found ID %d still causing yellow in track order (right%d), et=%.2f\n", trp.mID, k, info.mCurrentET );

          // Get previous and current total distance
          const double prevTotalDist = rightLane.mMember[ k ].mDist;
          const double curTotalDist = knownCar->CorrectedTotalLapDist( mFullLapDist );

          // Found it. Now see if any other vehicles from the frozen order have just passed it.
          long end = rightLane.mNumMembers;
          for( long w = 1; w < end; ++w )
          {
            const long j = ( k + w ) % rightLane.mNumMembers;

            // Get opponent's original and current total distance
            KnownCar *oppKnownCar = GetKnownCarByID( rightLane.mMember[ j ].mID );
            if( oppKnownCar != NULL )
            {
              if( ( 1 == mAllowFrozenAdjustments ) && ( mLogFile != NULL ) )
                fprintf( mLogFile, "Comparing ID %d against ID %d\n", trp.mID, oppKnownCar->mID );

              const double oppPrevTotalDist = rightLane.mMember[ j ].mDist;
              const double oppCurTotalDist = oppKnownCar->CorrectedTotalLapDist( mFullLapDist );

              // Have we gone another lap down?
              const long curLapsDown = static_cast< long >( floor( ( oppCurTotalDist - curTotalDist ) / mFullLapDist ) );
              const long origLapsDown = static_cast< long >( floor( ( oppPrevTotalDist - prevTotalDist ) / mFullLapDist ) );
              if( curLapsDown > origLapsDown )
              {
                if( mLogFile != NULL )
                {
                  fprintf( mLogFile, "ID %d (%.1f,%.1f) was legally passed by ID %d (%.1f,%.1f), fld=%f\n",
                           knownCar->mID, prevTotalDist, curTotalDist,
                           oppKnownCar->mID, oppPrevTotalDist, oppCurTotalDist, mFullLapDist );
                }

                // Swap positions in frozen order.
                // But make sure to keep a lead-lapper at the front of the line.
                if( ( k == 0 ) && ( oppKnownCar->mLapsDown > 0 ) )
                {
                  // Special case: If leader causes yellow and gets passed by someone a lap or more down, that someone gets to
                  // continue around and join at the end of the line.
                  // assert( j == 1 );
                  // assert( w == 1 );
                  for( long n = j; n < ( rightLane.mNumMembers - 1 ); ++n ) // 2nd guy swaps repeatedly until last
                  {
                    const Lane::Member swap = rightLane.mMember[ n ];
                    rightLane.mMember[ n ] = rightLane.mMember[ n + 1 ];
                    rightLane.mMember[ n + 1 ] = swap;
                  }

                  // Fix guy's lap down data
                  --oppKnownCar->mLapsDown;
                  for( long n = 0; n < info.mNumParticipants; ++n )
                  {
                    TrackRulesParticipantV01 &oppTrp = info.mParticipant[ n ];
                    if( oppKnownCar->mID == oppTrp.mID )
                    {
                      --oppTrp.mRelativeLaps;
                      break;
                    }
                  }

                  // Note: k shouldn't change because leader is left at the front
                }
                else if( k == ( rightLane.mNumMembers - 1 ) )
                {
                  // Special case: Last place fella must've been passed by leader causing yellow ... move his frozen order
                  // location behind the leader and a(nother?) lap down.
                  // assert( j == 0 );
                  for( long n = k - 1; n > j; --n ) // last guy swap repeatedly until 2nd
                  {
                    const Lane::Member swap = rightLane.mMember[ n ];
                    rightLane.mMember[ n ] = rightLane.mMember[ n + 1 ];
                    rightLane.mMember[ n + 1 ] = swap;
                  }

                  // Fix guy's lap down data
                  ++knownCar->mLapsDown;
                  ++trp.mRelativeLaps;

                  // Set k to new location
                  k = j + 1;
                }
                else
                {
                  // Normal swap!
                  // assert( j == ( k + 1 ) );
                  const Lane::Member swap = rightLane.mMember[ j ];
                  rightLane.mMember[ j ] = rightLane.mMember[ k ];
                  rightLane.mMember[ k ] = swap;

                  // Set k to j because that's where we swapped to
                  k = j;
                }

                // Start again with the vehicle right behind ...
                w = 0;

                // ... but don't swap with the same vehicle twice in the same cycle
                --end;
              }
              else
              {
                // If not behind the next guy in line, we should stop checking the remaining vehicles because if they're
                // ahead of us, they'll get swapped ahead of the next guy in line for no good reason!
                break;
              }
            }

            // UNDONE - I really don't get this right now ... it makes the w loop completely pointless, even though
            //          we reset w and change the end of the loop at one point ... or am I misreading this???
            // We're done
            break;
          }
        }
      }
    }
  }

  // Update all lap distances which will be the "previous" distance next time.
  for( long k = 0; k < rightLane.mNumMembers; ++k )
  {
    const KnownCar *knownCar = GetKnownCarByID( rightLane.mMember[ k ].mID );
    if( knownCar != NULL )
      rightLane.mMember[ k ].mDist = knownCar->CorrectedTotalLapDist( mFullLapDist );
  }
}


void StockCarRulesPlugin::RemoveFromOrder( const KnownCar *knownCar )
{
  if( knownCar == NULL )
  {
    fprintf( mLogFile, "ERROR: RemoveFromOrder( <null> )\n" );
    return;
  }

  if( mLogFile != NULL )
    fprintf( mLogFile, "Removing \"%s\" from track order\n", knownCar->mDriverName );

  for( long j = 0; j < TRCOL_MAX_LANES; ++j )
  {
    Lane &lane = mLane[ j ];
    for( long k = 0; k < lane.mNumMembers; ++k )
    {
      if( lane.mMember[ k ].mID == knownCar->mID )
      {
        // Move everybody up
        --lane.mNumMembers;
        for( long n = k; n < lane.mNumMembers; ++n )
          lane.mMember[ n ] = lane.mMember[ n + 1 ];
        lane.mMember[ lane.mNumMembers ].mID = -1; // invalidate (really just for debugging purposes)
      }
    }
  }

  ClearPassStatuses( knownCar );
}


bool StockCarRulesPlugin::AccessTrackRules( TrackRulesV01 &info )
{
  // Make sure there's nothing pending when we are asked to initialize the track order under caution
  if( TRSTAGE_CAUTION_INIT == info.mStage )
  {
    if( mLogFile != NULL )
      fprintf( mLogFile, "TRSTAGE_CAUTION_INIT:\n" );

    // Initialize some caution variables
    mProcessedLastLapOfCaution = false;
    mLuckyDogID = -1;
    mLeaderLaneChoiceLateral = DBL_MAX;
    mCautionET = info.mCurrentET;

    // Freeze the order now.
    FreezeOrder( info );
  }
  else if( TRSTAGE_FORMATION_INIT == info.mStage )
  {
    if( mLogFile != NULL )
      fprintf( mLogFile, "TRSTAGE_FORMATION_INIT:\n" );

    // Clear track order
    for( long i = 0; i < TRCOL_MAX_LANES; ++i )
      mLane[ i ].mNumMembers = 0;

    // Make sure these are clear (not that we adjust the frozen order on a formation lap!?)
    ClearAllPassStatuses();

    // Double file formation:
    // Loop through places, finding the vehicle in that place, and add it to the track order.
    const bool poleOnLeft = ( info.mPoleColumn == TRCOL_LEFT_LANE );
    for( long place = 1; place <= mNumKnownCars; ++place )
    {
      // Look for vehicle with this place
      for( long k = 0; k < mNumKnownCars; ++k )
      {
        const KnownCar &knownCar = mKnownCar[ k ];
        if( place == knownCar.mPlace )
        {
          // Add to track order
          Lane::Member laneMember;
          laneMember.mID = knownCar.mID;
          laneMember.mDist = knownCar.CorrectedTotalLapDist( mFullLapDist ); // probably not necessary for formation la, but ...
          if( ( ( place % 2 ) == 0 ) ^ poleOnLeft )
            AddMember( TRCOL_LEFT_LANE, laneMember );
          else
            AddMember( TRCOL_RIGHT_LANE, laneMember );
        }
      }
    }
  }

  // Keep track of mYellowSeverity around the time that a full-course caution is called. This logic records the 'worst' (highest) value
  // starting with the update just before the caution initialization and up to X seconds after the caution initialization. The plan is
  // to use that information to help determine if a car was involved in the incident (although it may not capture the case where one car
  // nudges another car into a spin).
  const double recordSeveritySeconds = 5.0;
  for( long i = 0; i < mNumKnownCars; ++i )
  {
    KnownCar &knownCar = mKnownCar[ i ];
    for( long j = 0; j < info.mNumParticipants; ++j )
    {
      const TrackRulesParticipantV01 &trp = info.mParticipant[ j ];
      if( knownCar.mID == trp.mID )
      {
        switch( info.mStage )
        {
          default:
          case TRSTAGE_NORMAL:
          {
            knownCar.mWorstYellowSeverity = trp.mYellowSeverity;
          }
          break;

          case TRSTAGE_CAUTION_INIT:
          {
            if( knownCar.mWorstYellowSeverity < trp.mYellowSeverity )
              knownCar.mWorstYellowSeverity = trp.mYellowSeverity;
          }
          break;

          case TRSTAGE_CAUTION_UPDATE:
          {
            if( info.mCurrentET < ( mCautionET + recordSeveritySeconds ) )
            {
              if( knownCar.mWorstYellowSeverity < trp.mYellowSeverity )
                knownCar.mWorstYellowSeverity = trp.mYellowSeverity;
            }
          }
          break;
        }
      }
    }
  }

  // Update the yellow flag state
  const long pitsClosedLaps = 0; // UNDONE - make this configurable or at least variable depending on circumstances?
  switch( info.mYellowFlagState ) // UNDONE - I wish the jerk who wrote the interface would learn about enums. Oops, that's me.
  {
    case 0: // none
    {
      if( info.mYellowFlagDetected )
      {
        info.mYellowFlagState = 1; // pending
        mYellowFlagLaps = 0; // this indicates we are undecided at this point
      }
    }
    break;

    case 1: // pending
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Wait until leader takes yellow
      for( long i = 0; i < info.mNumParticipants; ++i )
      {
        if( info.mParticipant[ i ].mPlace == 1 )
        {
          const KnownCar *knownCar = GetKnownCarByID( info.mParticipant[ i ].mID );
          if( knownCar->mUnderYellow )
            info.mYellowFlagState = 2; // pits closed
        }
      }
    }
    break;

    case 2: // pits closed
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Apply lucky free dog pass at some point once everybody is accounted for.
      // tag.2015.08.22 - Added a check for the finish status, because a car could be DNF'd or DQ'd without being in pits. I'm not going to worry too
      //                  much about a car that is finished, because the lucky dog at that point is kind of moot.
      if( mLuckyDogID < 0 )
      {
        long i;
        for( i = 0; i < mNumKnownCars; ++i )
        {
          const KnownCar &knownCar = mKnownCar[ i ];
          if( !knownCar.mUnderYellow && !knownCar.mInPits && ( knownCar.mFinishStatus < 2 ) ) // mFinishStatus will be: 0=none, 1=finished, 2=dnf, 3=dq
            break;
        }

        if( i == mNumKnownCars )
          ApplyLuckyDogFreePass( info );
      }

      // Change yellow flag state when the safety car approaches the pitlane
#if 0 // If clear yellow request happens now, this version immediately opens the pits to everybody ...
      if( SafetyCarIsApproachingPitlane( info, pitsClosedLaps ) ||  // if safety car approaching on last closed-pit lap, or ...
          ( 2 == info.mYellowFlagLapsWasOverridden ) )              // ... admin request to clear yellow
#else // ... while this version keeps pits closed until the safety car approaches pitlane.
      if( SafetyCarIsApproachingPitlane( info, pitsClosedLaps ) )
#endif
      {
        // If we haven't decided on the number of yellow flag laps yet, decide now!
        // UNDONE - choose quickie or normal here based on something or another
        if( mYellowFlagLaps == 0 )
        {
          if( 0 != info.mYellowFlagLapsWasOverridden )
            mYellowFlagLaps = info.mYellowFlagLaps;
          else
            mYellowFlagLaps = 4; // UNDONE - based upon the severity or something
        }

        if( 2 == info.mYellowFlagLapsWasOverridden ) // admin request to clear yellow
          info.mYellowFlagState = 4; // pits open
        else
          info.mYellowFlagState = 3; // pit lead lap
      }
    }
    break;

    case 3: // pit lead lap
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Apply lucky dog if was never awarded during the previous yellow flag state.
      if( mLuckyDogID < 0 )
        ApplyLuckyDogFreePass( info );

      // Change yellow flag state when the safety car approaches the pitlane
#if 1 // If clear yellow request happens now, this version immediately opens the pits to everybody ...
      if( SafetyCarIsApproachingPitlane( info, pitsClosedLaps + 1 ) ||  // if safety car approaching after one lead-lappers-only-pit lap, or ...
          ( 2 == info.mYellowFlagLapsWasOverridden ) )                  // ... admin request to clear yellow
#else // ... while this version keeps lead-lappers-only-pit until the safety car approaches pitlane. But I think it also delays the "one lap to go" message until rather late!?
      if( SafetyCarIsApproachingPitlane( info, pitsClosedLaps ) )
#endif
      {
        info.mYellowFlagState = 4; // pits open
      }
    }
    break;

    case 4: // pits open
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Admin request to clear yellow:
      if( 2 == info.mYellowFlagLapsWasOverridden )
        mYellowFlagLaps = info.mYellowFlagLaps;

      // Change yellow flag state when the safety car is on the last lap
      if( info.mSafetyCarLaps >= ( mYellowFlagLaps - 1 ) )
      {
        // Convert to double-file (okay, this is done below the switch/case)

        // Decipher whether leader wants to be in the left or right lane.
        const KnownCar *knownCar = GetFirstCarInRightLaneThatIsOnTheLeadLap(); // first member in right lane that is not a lap down is the leader
        if( knownCar != NULL )
        {
          // Because VehicleScoringInfoV01::mPathLateral is not particularly accurate, we will judge the desired
          // lateral position by weighting two factors:
          //   1) which direction is the current lateral position compared to the "center" path, and
          //   2) how much it has changed since we requested the leader to choose a lane.
          if( DBL_MAX == mLeaderLaneChoiceLateral )
            mLeaderLaneChoiceLateral = knownCar->mPathLateral; // hmm, for some reason we didn't record their position when we requested the lane choice (if we even did that)
          else
            mLeaderLaneChoiceLateral = ( 2.0 * knownCar->mPathLateral ) - mLeaderLaneChoiceLateral;
        }

        // Move to next state
        info.mYellowFlagState = 5; // last lap
      }
      else if( info.mSafetyCarLaps >= ( mYellowFlagLaps - 2 ) )
      {
        // If we have not already done so, and we are within a few seconds of starting the last lap, then
        // request the leader to make a lane choice.
        if( mLeaderLaneChoiceLateral == DBL_MAX )
        {
          if( ( info.mSafetyCarLapDist + ( 5.0f * info.mSafetyCarSpeed ) ) > mFullLapDist )
          {
            // First member in right lane that is not a lap down is the leader
            const KnownCar *knownCar = GetFirstCarInRightLaneThatIsOnTheLeadLap();
            if( knownCar != NULL )
            {
              // Record leader's current lateral position, for comparison when we decipher the choice.
              mLeaderLaneChoiceLateral = knownCar->mPathLateral;
              for( long k = 0; k < info.mNumParticipants; ++k )
              {
                TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
                if( knownCar->mID == trp.mID )
                {
                  strcpy_s( trp.mMessage, "Choose A Lane By Staying Left Or Right" );
                  break;
                }
              }
            }
          }
        }
      }
    }
    break;

    case 5: // last lap
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Set random lap distance if necessary
      if( mRandomGreenDist < 0.0f )
      {
        // Use fractions of a meter as a random value from 0.0-1.0 ... (cheap and easy, and possibly even random)
        double sum = 0.0;        
        for( long j = 0; j < mNumKnownCars; ++j )
        {
          const KnownCar &knownCar = mKnownCar[ j ];
          sum += ( ( j + 0.5 ) * mKnownCar[ j ].mLapDist ); // give different weights to different vehicles to make it "more" "random"
        }
        sum = fabs( sum - static_cast< long >( sum ) ); // take the fraction
        mRandomGreenDist = 50.0f + ( 50.0f * static_cast< float >( sum ) );
      }

      // Decide when to throw green
      if( !info.mSafetyCarActive )
      {
        // Look for any vehicle to be past the front teleport location
        // tag.2015.08.22 - Checking to make sure the vehicle isn't DNF'd or DQ'd.
        for( long j = 0; j < mNumKnownCars; ++j )
        {
          const KnownCar &knownCar = mKnownCar[ j ];
          if( !knownCar.mInPits && ( knownCar.mFinishStatus < 2 ) ) // mFinishStatus will be: 0=none, 1=finished, 2=dnf, 3=dq
          {
            const float compareLapDist = static_cast< float >( knownCar.mLapDist );
            if( compareLapDist > ( info.mTeleportLapDist + mRandomGreenDist ) )
            {
              info.mYellowFlagState = 6; // resume
              mRandomGreenDist = -1.0f; // signal to re-randomize it next time we need it
              break;
            }
          }
        }
      }
    }
    break;

    case 6: // resume
    {
      // Allow the frozen order to be adjusted as spun vehicles are passed
      if( info.mYellowFlagState < mAdjustUntilYellowFlagState )
        AdjustFrozenOrder( info );

      // Wait until leader is no longer yellow
      for( long i = 0; i < info.mNumParticipants; ++i )
      {
        if( info.mParticipant[ i ].mPlace == 1 )
        {
          const KnownCar *knownCar = GetKnownCarByID( info.mParticipant[ i ].mID );
          if( knownCar != NULL )
          {
            if( !knownCar->mUnderYellow )
              info.mYellowFlagState = 0; // none
          }
        }
      }
    }
    break;

    case 7: // race halt
    {
      // not implemented
    }
    break;
  }

  // Beyond this, we're not doing anything except handling full-course yellow updates to the track order
  if( TRSTAGE_CAUTION_UPDATE != info.mStage )
  {
    // UNDONE - For efficienty, consider clearing the track order only if the stage *just* changed to normal.
    bool ret = false;
    if( TRSTAGE_NORMAL == info.mStage )
    {
      // Clear out track order under normal racing; otherwise the start of a full-course caution might show an incorrect message.
      for( long i = 0; i < info.mNumParticipants; ++i )
      {
        if( ( info.mParticipant[ i ].mColumnAssignment != TRCOL_INVALID ) ||
            ( info.mParticipant[ i ].mPositionAssignment != -1 ) )
        {
          info.mParticipant[ i ].mColumnAssignment = TRCOL_INVALID;
          info.mParticipant[ i ].mPositionAssignment = -1;
          ret = true;
        }
      }
    }

    // Actually, we always have to return true if we want to control the yellow flag state ...
//    return( ret );
    return( true );
  }

  // Include additions
  for( long i = 0; i < info.mNumActions; ++i )
  {
    // Store and delay these actions to ensure correct order.
    if( mNumActions >= mMaxActions )
    {
      TrackRulesActionV01 *newAction = new TrackRulesActionV01[ ++mMaxActions ];
      for( long j = 0; j < mNumActions; ++j )
        newAction[ j ] = mAction[ j ];
      delete [] mAction;
      mAction = newAction;
    }
    mAction[ mNumActions++ ] = info.mAction[ i ];
  }

  // Sort delayed actions by time, with the earlier ones *LAST*. (There usually won't be more than one or two, so simple bubble sort is fine.)
  for( long i = 0; i < mNumActions; ++i )
  {
    for( long j = 0; j < mNumActions; ++j )
    {
      for( long k = 1; k < mNumActions; ++k )
      {
        if( mAction[ k - 1 ].mET < mAction[ k ].mET )
        {
          const TrackRulesActionV01 swap = mAction[ k - 1 ];
          mAction[ k - 1 ] = mAction[ k ];
          mAction[ k ] = swap;
        }
      }
    }
  }

  // If action is old enough, perform it. Since we sorted the earlier actions last, just step through backwards ...
  const double etThresh = info.mCurrentET - 1.0; // one second is long enough to handle most reasonable net lag
  for( long i = mNumActions - 1; i >= 0; --i )
  {
    // Bail out of loop if it's too young
    TrackRulesActionV01 &action = mAction[ i ];
    if( action.mET >= etThresh )
      break;

    // Moved this logging code below the ET comparison above. It was too annoying/confusing having the same action
    // reported several times before it was ready to be processed (although it did prove the mechanism worked).
    if( mLogFile != NULL )
      fprintf( mLogFile, "Examining action=%d id=%d et=%.3f thresh=%.3f\n", action.mCommand, action.mID, action.mET, etThresh );

    // Perform action
    switch( action.mCommand )
    {
      case TRCMD_ADD_FROM_PIT:
      case TRCMD_ADD_FROM_UNDQ:
      {
        // Put in track order somewhere
        const KnownCar *knownCar = GetKnownCarByID( action.mID );
        if( knownCar != NULL )
        {
          for( long n = 0; n < info.mNumParticipants; ++n )
          {
            TrackRulesParticipantV01 &trp = info.mParticipant[ n ];
            if( knownCar->mID == trp.mID )
            {
              //
              if( mLogFile != NULL )
                fprintf( mLogFile, "Now adding \"%s\" back to order (action %d), relative laps is %d\n", knownCar->mDriverName, action.mCommand, trp.mRelativeLaps );

              //
              Lane::Member laneMember;
              laneMember.mID = knownCar->mID;
              laneMember.mDist = knownCar->CorrectedTotalLapDist( mFullLapDist );

              // Initialize at the end of the right lane (this will allocate memory if necessary), with no pass statuses recorded
              Lane &rightLane = mLane[ TRCOL_RIGHT_LANE ];
              AddMember( TRCOL_RIGHT_LANE, laneMember );
              ClearPassStatuses( knownCar );

#if 0 // not sure how at the moment, but trp.mRelativeLaps already seems to be correct depending on whether we are ahead (rl=-1) or behind (rl=0) leader on track
              // But adjust relative laps if going to end.
              const KnownCar *leader = GetKnownCarByID( rightLane.mMember[ 0 ].mID );
              if( knownCar->mLapDist > leader->mLapDist )
              {
                --trp.mRelativeLaps;
                if( mLogFile != NULL )
                  fprintf( mLogFile, "While adding \"%s\" back to order (action %d), adjusted relative laps to %d due to being ahead of leader on track\n", knownCar->mDriverName, action.mCommand, trp.mRelativeLaps );
                break;
              }
#endif

              // And correct the PassStatus matrix depending on where we are.
              const double myRelativeLapDist = knownCar->mLapDist + ( trp.mRelativeLaps * mFullLapDist );
              for( long k = 0; k < ( rightLane.mNumMembers - 1 ); ++k )
              {
                const KnownCar *oppKnownCar = GetKnownCarByID( rightLane.mMember[ k ].mID );
                for( long m = 0; m < info.mNumParticipants; ++m )
                {
                  TrackRulesParticipantV01 &oppTrp = info.mParticipant[ m ];
                  if( oppKnownCar->mID == oppTrp.mID )
                  {
                    const double theirRelativeLapDist = oppKnownCar->mLapDist + ( oppTrp.mRelativeLaps * mFullLapDist );
                    if( myRelativeLapDist > theirRelativeLapDist )
                      SetPassStatus( knownCar, oppKnownCar, PS_PHYSICAL );
                  }
                }
              }

              //
              if( mLogFile != NULL )
                fprintf( mLogFile, "Finished adding \"%s\" back to order\n", knownCar->mDriverName );
            }
          }
        }
      }
      break;

      case TRCMD_ADD_FROM_TRACK:
      {
        // With frozen order, we don't need to process this command.
      }
      break;

      case TRCMD_REMOVE_TO_DNF:
      case TRCMD_REMOVE_TO_DQ:
      case TRCMD_REMOVE_TO_PIT:
      case TRCMD_REMOVE_TO_UNLOADED:
      {
        // Remove from track order
        const KnownCar *knownCar = GetKnownCarByID( action.mID );
        if( knownCar != NULL )
          RemoveFromOrder( knownCar );
      }
      break;

      case TRCMD_MOVE_TO_BACK: // UNDONE - incorrectly applying the same as longest line right now ...
      case TRCMD_LONGEST_LINE:
      {
        // Add a longest line penalty
        KnownCar *knownCar = GetKnownCarByID( action.mID );
        if( knownCar != NULL )
          ++( knownCar->mNumEOLLs );
      }
      break;
    }

    // Decrement because we have now used this one (and we're looping backwards)
    --mNumActions;
    action.mCommand = TRCMD_MAXIMUM; // invalidate for debugging purposes
  }

  // tag.2015.08.19 - Always set the mUpToSpeed flag under yellow now; based on results, it was apparently getting missed in some cases before.
  if( 0 != mYellowFlagState )
  {
    // tag.2015.06.?? - This isn't working well at the moment. If you hold off on telling people who to be behind, they get confused (particularly
    //                  the AI). I guess it's better to occasionally change it, then to hold off.
    // tag.2015.07.07 - Turning this back on, but adjusting algorithm to give the best guess at who you'll be following. Yes, it may change if the
    //                  car causing yellow gets back up to speed, but it's impossible to know if and when that will happen so this is the best we
    //                  can do. I think. UNDONE - Ideally, we should add hysterisis rather than having a fixed threshold.
    float threshold;
    if( mYellowFlagState < 2 )
      threshold = 0.25f * mAdjustFrozenOrder * info.mSafetyCarThreshold; // smaller threshold needed than for adjusting the frozen order
    else if( mYellowFlagState < 3 )
      threshold = 0.75f * mAdjustFrozenOrder * info.mSafetyCarThreshold; // smaller threshold needed than for adjusting the frozen order
    else
      threshold = FLT_MAX;

    for( long k = 0; k < info.mNumParticipants; ++k )
    {
      TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
      trp.mUpToSpeed = ( trp.mYellowSeverity < threshold );
      trp.mPitsOpen = ( ( mYellowFlagState >= 4 ) && ( mYellowFlagState <= 6 ) ); // pits are open for the following states: "pits open" (duh), "last lap", or "resume"
    }

    // Extra logic to determine mPitsOpen when the yellow flag state is set to "pit lead lap"
    if( 3 == mYellowFlagState )
    {
      for( long k = 0; k < info.mNumParticipants; ++k )
      {
        TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
        const KnownCar *knownCar = GetKnownCarByID( trp.mID );
        if( knownCar != NULL )
          trp.mPitsOpen = ( 0 == knownCar->mLapsDown );
      }
    }
  }

  // The rest depends on where we are in the caution
  if( mYellowFlagState == 5 ) // 5 means "last lap"
  {
    if( mProcessedLastLapOfCaution )
    {
      // Make sure everybody is following orders
    }
    else
    {
      // Change to double file ... previously all the cars should have been put in the right lane, which we'll copy here:
      Lane orig;
      for( long i = 0; i < mLane[ TRCOL_RIGHT_LANE ].mNumMembers; ++i )
        orig.AddMember( mLane[ TRCOL_RIGHT_LANE ].mMember[ i ] );

      mLane[ TRCOL_RIGHT_LANE ].mNumMembers = 0;
      for( long i = 0; i < TRCOL_MAX_LANES; ++i )
      {
        if( ( 0 != mLane[ i ].mNumMembers ) && ( mLogFile != NULL ) )
          fprintf( mLogFile, "Lane %d had %d members before re-population, should have been zero!", i, mLane[ i ].mNumMembers );
        mLane[ i ].mNumMembers = 0;
      }

      // Sort cars into bins to determine where they go:
      // (D<x> category refers to internal document category.)
      // 0 = D1 = lead lap vehicles that didn't pit
      // 1 = D2 = lead lap vehicles that pitted once at the appropriate time
      // 2 = D3 = lapped vehicles that did not pit
      // 3 = D4 = lapped vehicles that pitted at appropriate time, or cars that pitted more than once
      // 4 = D5 = lucky dog (i.e. beneficiary, the highest scored non-lead lap car at time of caution)
      // 5 = D6 = wave around cars
      // 6 = D7 = ???
      // 7 = D8 = ???
      const long numBins = 8;
      long binSize[ numBins ];
      unsigned char *bin[ numBins ];
      for( long i = 0; i < numBins; ++i )
      {
        binSize[ i ] = 0;
        bin[ i ] = new unsigned char[ orig.mNumMembers ]; // this is the maximum size that could be added to any one bin
      }

      // First I need to know who the current leader is ... which is the first member on the lead lap
      long leaderMemberIndex = 0;
      for( long i = 0; i < orig.mNumMembers; ++i )
      {
        const long id = orig.mMember[ i ].mID;

        // Find this known car in our list
        const KnownCar *knownCar = GetKnownCarByID( id );
        if( knownCar != NULL )
        {
          if( 0 == knownCar->mLapsDown )
          {
            leaderMemberIndex = i;
            break;
          }
        }
      }

      // Process all vehicles that are currently out on track ... note that these additions are currently
      // in order, so by processing them in order we maintain their order within each bin.
      for( long i = 0; i < orig.mNumMembers; ++i )
      {
        const long id = orig.mMember[ i ].mID;

        // Find this known car in our list
        long j;
        for( j = 0; j < mNumKnownCars; ++j )
        {
          KnownCar &knownCar = mKnownCar[ j ];
          if( id == knownCar.mID )
          {
            // Which bin? (D<x> category refers to internal document category)
            long binIndex = 7;
            bool waveAround = false;
            bool eoll = false;

            // Put into the appropriate bin
            if( 0 < knownCar.mNumEOLLs )
            {
              binIndex = 6;
              eoll = true;
            }
            else if( id == mLuckyDogID )
            {
              binIndex = 4;
            }
            else if( 0 == knownCar.mLapsDown ) // lead lap
            {
              if( knownCar.mPittedAtSomeOtherTime )
                binIndex = 3; // knownCar.mPittedAtDesignatedTime is irrelevant in this case
              else
                binIndex = knownCar.mPittedAtDesignatedTime ? 1 : 0;
            }
            else // lapped vehicles
            {
              if( knownCar.mPittedAtDesignatedTime )
              {
                binIndex = 3; // knownCar.mPittedAtSomeOtherTime is irrelevant in this case
              }
              else
              {
                if( knownCar.mPittedAtSomeOtherTime )
                {
                  binIndex = 3;
                }
                else
                {
                  if( i < leaderMemberIndex )
                  {
                    binIndex = 5;
                    waveAround = true;
                  }
                  else
                  {
                    binIndex = 2;
                  }
                }
              }
            }

            // Find this known car in the interface
            long k;
            for( k = 0; k < info.mNumParticipants; ++k )
            {
              if( id == info.mParticipant[ k ].mID )
                break;
            }

            if( k < info.mNumParticipants )
            {
              TrackRulesParticipantV01 &trp = info.mParticipant[ k ];

              // Put in bin, give message if necessary
              if( waveAround )
              {
                --trp.mRelativeLaps;
                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "Wave Around: Pass Field On Left" );
              }
              else if( eoll )
              {
                --knownCar.mNumEOLLs;
                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "Move To End Of Longest Line" );
              }
              else if( 0 == binIndex )
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Lead lap, didn't pit (D1)" );
              }
              else if( 1 == binIndex )
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Lead lap, pit once (D2)" );
              }
              else if( 2 == binIndex )
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Lapped, didn't pit (D3)" );
              }
              else if( 3 == binIndex )
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Various (category D4)" );
              }
              else if( 4 == binIndex )
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Lucky free pass dog (category D5)" );
              }
              else
              {
//                strcpy_s( trp.mMessage, sizeof( trp.mMessage ), "TEST MSG: Unknown! BUG!" );
              }

              if( mLogFile != NULL )
              {
                fprintf( mLogFile, "ID=%2d patd=%d pasot=%d lapsdown=%d rellaps=%d lapdist=%6.1f waveAround=%d bin=%d, et=%.2f\n", trp.mID, knownCar.mPittedAtDesignatedTime,
                         knownCar.mPittedAtSomeOtherTime, knownCar.mLapsDown, trp.mRelativeLaps, knownCar.mLapDist, waveAround, binIndex, info.mCurrentET );
              }

              bin[ binIndex ][ binSize[ binIndex ]++ ] = static_cast< unsigned char >( id );
            }
            else
            {
              if( mLogFile != NULL )
                fprintf( mLogFile, "ERROR: couldn't find addition in interface list of known cars!\n" );
            }

            //
            break;
          }
        }

        if( j == mNumKnownCars )
        {
          if( mLogFile != NULL )
            fprintf( mLogFile, "ERROR: couldn't find addition in our list of known cars!\n" );
        }
      }

      if( mLogFile != NULL )
      {
        fprintf( mLogFile, "BEFORE final lap rearrangement:\n" );
        for( long k = 0; k < info.mNumParticipants; ++k )
        {
          const TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
          fprintf( mLogFile, " id=%2d column=%d position=%d uts=%d po=%d\n", trp.mID, trp.mColumnAssignment, trp.mPositionAssignment, trp.mUpToSpeed, trp.mPitsOpen );
        }
        fprintf( mLogFile, "AFTER final lap rearrangement:\n" );
      }

      // Clear out any existing known car info
      for( long i = 0; i < info.mNumParticipants; ++i )
      {
        info.mParticipant[ i ].mColumnAssignment = TRCOL_INVALID;
        info.mParticipant[ i ].mPositionAssignment = -1;
      }

      // UNDONE - What do we do about outstanding pass statuses? As of this writing, it doesn't matter because we don't
      //          adjust the frozen at this stage in the caution. But if we did, I think the best answer is that we leave the
      //          pass status matrix as is. If car A illegally passed car B but then legally passed car C, and then they were
      //          re-arranged into a double file with A behind C, I think it's okay that the leftover pass status promotes A
      //          past C. Right?

      // Now re-add from bins
      long numAdded = 0;
      for( long i = 0; i < numBins; ++i )
      {
        for( long j = 0; j < binSize[ i ]; ++j )
        {
          const long id = bin[ i ][ j ];

          // Find known car
          for( long k = 0; k < info.mNumParticipants; ++k )
          {
            TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
            if( id == trp.mID )
            {
              // First place gets lane choice, second place takes other lane, but the rest follow a standard pattern
              // regardless of the first two folks.
              // Note that lateral values are negative for left and positive for right (in this case).
              ++numAdded;
              if(      1 == numAdded )
                trp.mColumnAssignment = ( mLeaderLaneChoiceLateral <  0.0 ) ? TRCOL_LEFT_LANE : TRCOL_RIGHT_LANE;
              else if( 2 == numAdded )
                trp.mColumnAssignment = ( mLeaderLaneChoiceLateral >= 0.0 ) ? TRCOL_LEFT_LANE : TRCOL_RIGHT_LANE;
              else
                trp.mColumnAssignment = (                 numAdded &  0x1 ) ? TRCOL_LEFT_LANE : TRCOL_RIGHT_LANE;

              trp.mPositionAssignment = mLane[ trp.mColumnAssignment ].mNumMembers;

              if( mLogFile != NULL )
                fprintf( mLogFile, " id=%2d column=%d position=%d uts=%d po=%d\n", trp.mID, trp.mColumnAssignment, trp.mPositionAssignment, trp.mUpToSpeed, trp.mPitsOpen );

              Lane::Member laneMember;
              laneMember.mID = trp.mID;
              laneMember.mDist = -1.0; // We only need this for sorting the frozen order for the first lap or two of caution. If we need it here, then we need to find the known car info!
              AddMember( trp.mColumnAssignment, laneMember );
            }
          }
        }
      }

      // de-allocate
      for( long i = 0; i < numBins; ++i )
      {
        delete [] bin[ i ];
        bin[ i ] = NULL;
      }

      // Set flag so we know we already did this.
      mProcessedLastLapOfCaution = true;
    }
  }
  else if( mYellowFlagState < 5 )
  {
    // Clear out any existing lane info
    for( long i = 0; i < info.mNumParticipants; ++i )
    {
      info.mParticipant[ i ].mColumnAssignment = TRCOL_INVALID;
      info.mParticipant[ i ].mPositionAssignment = -1;
    }

    // Fill in from our lane info, starting a few seconds after the caution came out, perhaps allowing the field to settle a bit from
    // the potential chaos of an accident resulting in a full-course caution, but also to too much info being splashed all at one time
    // to an overwhelmed user.
    // Update: Changed from 5.0 seconds to 1.5 seconds, because if the safety car pops out just before the leader crosses the s/f line
    // with a full head of steam, there won't be enough time to react. This should probably be tested to see if even this is enough time.
    if( info.mCurrentET > ( mCautionET + 1.5 ) )
    {
      for( long i = 0; i < TRCOL_MAX_LANES; ++i )
      {
        const Lane &lane = mLane[ i ];
        long positionAssignment = 0;
        for( long j = 0; j < lane.mNumMembers; ++j )
        {
          const long id = lane.mMember[ j ].mID;

          long k;
          for( k = 0; k < info.mNumParticipants; ++k )
          {
            TrackRulesParticipantV01 &trp = info.mParticipant[ k ];
            if( trp.mID == id )
            {
//              // If this vehicle is still causing a yellow and has not crossed the s/f line under this yellow, let's hold off on
//              // telling everybody behind them who they're supposed to be following ...
//              // tag.2015.07.07 - See comment above, changed from a 'break' to a 'continue' here.
//              // tag.2015.07.14 - Changing this again. Yellow-causing vehicle stays in order for now, but mUpToSpeed flag gets turned off so people don't follow it.
//              // tag.2015.08.19 - Now setting mUpToSpeed flag all the time under yellow (somewhere else, probably above).
//              if( trp.mYellowSeverity > threshold )
//                continue;

              trp.mColumnAssignment = static_cast< TrackRulesColumnV01 >( i );
              trp.mPositionAssignment = positionAssignment++;
            }
          }

//          // This would only do something if there was some sort of break from that loop
//          if( k < info.mNumParticipants )
//            break;
        }
      }
    }
  }

  // Return true to indicate that we want to edit the track order
  return( true );
}


bool StockCarRulesPlugin::GetCustomVariable( long i, CustomVariableV01 &var )
{
  switch( i )
  {
    case 0:
    {
      // rF2 will automatically create this variable and default it to 1 (true) unless we create it first, in which case we can choose the default.
      strcpy_s( var.mCaption, " Enabled" );
      var.mNumSettings = 2;
      var.mCurrentSetting = 0;
      return( true );
    }
    // return before break;

    case 1:
    {
      strcpy_s( var.mCaption, "AllowFrozenAdjustments" );
      var.mNumSettings = 3;
      var.mCurrentSetting = 1;
      return( true );
    }
    // return before break;

    case 2:
    {
      strcpy_s( var.mCaption, "AdjustFrozenOrder" );
      var.mNumSettings = 200;
      var.mCurrentSetting = 25;
      return( true );
    }
    // return before break;

    case 3:
    {
      strcpy_s( var.mCaption, "AdjustUntilYellowFlagState" );
      var.mNumSettings = 9;
      var.mCurrentSetting = 4;
      return( true );
    }
    // return before break;

    case 4:
    {
      strcpy_s( var.mCaption, "Logging" );
      var.mNumSettings = 2;
      var.mCurrentSetting = 0;
      return( true );
    }
    // return before break;

    case 5:
    {
      strcpy_s( var.mCaption, "LuckyDogFreePass" );
      var.mNumSettings = 2;
      var.mCurrentSetting = 1;
      return( true );
    }
    // return before break;
  }

  return( false );
}


void StockCarRulesPlugin::AccessCustomVariable( CustomVariableV01 &var )
{
  if( 0 == _stricmp( var.mCaption, " Enabled" ) )
  {
    // Do nothing; this variable is just for rF2 to know whether to keep the plugin loaded.
  }
  else if( 0 == _stricmp( var.mCaption, "AllowFrozenAdjustments" ) )
  {
    mAllowFrozenAdjustments = var.mCurrentSetting;
  }
  else if( 0 == _stricmp( var.mCaption, "AdjustFrozenOrder" ) )
  {
    mAdjustFrozenOrder = var.mCurrentSetting / 100.0f;
  }
  else if( 0 == _stricmp( var.mCaption, "AdjustUntilYellowFlagState" ) )
  {
    mAdjustUntilYellowFlagState = var.mCurrentSetting;
  }
  else if( 0 == _stricmp( var.mCaption, "Logging" ) )
  {
    // Set a flag whether we want logging enabled ... it may be too early to actually open the file because
    // the call SetEnvironment() may not have been called yet.
    mLogging = ( var.mCurrentSetting != 0 );
  }
  else if( 0 == _stricmp( var.mCaption, "LuckyDogFreePass" ) )
  {
    mEnableLuckyDogFreePass = ( var.mCurrentSetting != 0 );
  }
  else
  {
  }
}


void StockCarRulesPlugin::GetCustomVariableSetting( CustomVariableV01 &var, long i, CustomSettingV01 &setting )
{
  if( 0 == _stricmp( var.mCaption, " Enabled" ) )
  {
    if( 0 == i )
      strcpy_s( setting.mName, "False" );
    else
      strcpy_s( setting.mName, "True" );
  }
  else if( 0 == _stricmp( var.mCaption, "AllowFrozenAdjustment" ) )
  {
    if( 0 == i )
      sprintf_s( setting.mName, "No" );
    else if( 1 == i )
      sprintf_s( setting.mName, "For Spun Cars" );
    else
      sprintf_s( setting.mName, "For Any Pass" );
  }
  else if( 0 == _stricmp( var.mCaption, "AdjustFrozenOrder" ) )
  {
    sprintf_s( setting.mName, "%d%%", i );
  }
  else if( 0 == _stricmp( var.mCaption, "AdjustUntilYellowFlagState" ) )
  {
    switch( i )
    {
      case 0:  sprintf_s( setting.mName, "Never"        ); break; // Yes, these two are functionally equivalent, because we never
      case 1:  sprintf_s( setting.mName, "Never"        ); break; // even try to adjust under green.
      case 2:  sprintf_s( setting.mName, "Pending"      ); break;
      case 3:  sprintf_s( setting.mName, "Pits Closed"  ); break;
      case 4:  sprintf_s( setting.mName, "Pit Lead Lap" ); break;
      case 5:  sprintf_s( setting.mName, "Pits Open"    ); break;
      case 6:  sprintf_s( setting.mName, "Last Lap"     ); break;
      case 7:  sprintf_s( setting.mName, "Resume"       ); break;
      case 8:
      default: sprintf_s( setting.mName, "Race Halt"    ); break; // We don't actually try to adjust during race halt currently.
    }
  }
  else if( 0 == _stricmp( var.mCaption, "Logging" ) )
  {
    if( 0 == i )
      strcpy_s( setting.mName, "False" );
    else
      strcpy_s( setting.mName, "True" );
  }
  else if( 0 == _stricmp( var.mCaption, "LuckyDogFreePass" ) )
  {
    if( 0 == i )
      strcpy_s( setting.mName, "False" );
    else
      strcpy_s( setting.mName, "True" );
  }
}


