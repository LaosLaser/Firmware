#ifndef LAOSEXTENTH
#define LAOSEXTENTH

#include "global.h"
#include "pins.h"
#include "planner.h"

// forward decls:
class LaosMotion;

    /** Get the minimum and maximum coordinates of a given file
      *
      * Example:
      * @code 
      * @endcode
      */
class LaosExtent {

public:
	typedef enum {errNone=0, errCoordReferenceChanged, errFileFormatError, errEmpty} TError;
	LaosExtent();
	void Reset(bool onlyMovesWithLaserOn);       // reset state machine
	void Write(int i);  // feed a simplecode value
	// get the boundaries
	// result is value only if the returned error code is errNone
	TError GetBoundary(int &minx, int &miny, int &maxx, int &maxy) const;
	// show boundaries by moving the head:
	void ShowBoundaries(LaosMotion *mot) const;

private:
	void AddToBoundary(int x, int y);

private:
	int m_MinX, m_MaxX, m_MinY, m_MaxY;  // boundaries (multiplied by 1000)
	bool m_HasMinMaxCoordinates;         // initially false; will be set true once the laser fires
	int m_TargetX, m_TargetY;            // target pos of current command
	int m_BitmapSize;
	int m_BitmapBpp;
	int m_Step;
	int m_Command;
	bool m_OnlyMovesWithLaserOn;
	TError m_Error;
};

#endif

