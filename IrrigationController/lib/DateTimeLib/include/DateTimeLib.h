/**************************************************************************/
/*!
  @file     RTClib.h

  Original library by JeeLabs http://news.jeelabs.org/code/, released to the
  public domain

  License: MIT (see LICENSE)

  This is a fork of JeeLab's fantastic real time clock library for Arduino.

  This is the subset of classes from RTClib https://github.com/adafruit/RTClib

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
*/
/**************************************************************************/

#ifndef _DATETIMELIB_H_
#define _DATETIMELIB_H_

#include <Arduino.h>


/** Constants */
#define SECONDS_PER_DAY 86400L ///< 60 * 60 * 24
#define SECONDS_FROM_1970_TO_2000 \
    946684800 ///< Unixtime for 2000-01-01 00:00:00, useful for initialization

/**************************************************************************/
/*!
    @brief  Timespan which can represent changes in time with seconds accuracy.
*/
/**************************************************************************/
class TimeSpan
{
public:
    TimeSpan(int32_t seconds = 0);
    TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
    TimeSpan(const TimeSpan &copy);

    /*!
        @brief  Number of days in the TimeSpan
                e.g. 4
        @return int16_t days
    */
    int16_t days() const { return _seconds / 86400L; }
    /*!
        @brief  Number of hours in the TimeSpan
                This is not the total hours, it includes the days
                e.g. 4 days, 3 hours - NOT 99 hours
        @return int8_t hours
    */
    int8_t hours() const { return _seconds / 3600 % 24; }
    /*!
        @brief  Number of minutes in the TimeSpan
                This is not the total minutes, it includes days/hours
                e.g. 4 days, 3 hours, 27 minutes
        @return int8_t minutes
    */
    int8_t minutes() const { return _seconds / 60 % 60; }
    /*!
        @brief  Number of seconds in the TimeSpan
                This is not the total seconds, it includes the days/hours/minutes
                e.g. 4 days, 3 hours, 27 minutes, 7 seconds
        @return int8_t seconds
    */
    int8_t seconds() const { return _seconds % 60; }
    /*!
        @brief  Total number of seconds in the TimeSpan, e.g. 358027
        @return int32_t seconds
    */
    int32_t totalseconds() const { return _seconds; }

    TimeSpan operator+(const TimeSpan &right) const;
    TimeSpan operator-(const TimeSpan &right) const;

protected:
    int32_t _seconds; ///< Actual TimeSpan value is stored as seconds
};

/**************************************************************************/
/*!
    @brief  Simple general-purpose date/time class (no TZ / DST / leap
            seconds).

    This class stores date and time information in a broken-down form, as a
    tuple (year, month, day, hour, minute, second). The day of the week is
    not stored, but computed on request. The class has no notion of time
    zones, daylight saving time, or
    [leap seconds](http://en.wikipedia.org/wiki/Leap_second): time is stored
    in whatever time zone the user chooses to use.

    The class supports dates in the range from 1 Jan 2000 to 31 Dec 2099
    inclusive.
*/
/**************************************************************************/
class DateTime
{
public:
    DateTime(uint32_t t = SECONDS_FROM_1970_TO_2000);
    DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0,
             uint8_t min = 0, uint8_t sec = 0);
    DateTime(const DateTime &copy);
    DateTime(const char *date, const char *time);
    DateTime(const __FlashStringHelper *date, const __FlashStringHelper *time);
    DateTime(const char *iso8601date);
    bool isValid() const;
    char *toString(char *buffer) const;

    /*!
        @brief  Return the year.
        @return Year (range: 2000--2099).
    */
    uint16_t year() const { return 2000U + yOff; }
    /*!
        @brief  Return the month.
        @return Month number (1--12).
    */
    uint8_t month() const { return m; }
    /*!
        @brief  Return the day of the month.
        @return Day of the month (1--31).
    */
    uint8_t day() const { return d; }
    /*!
        @brief  Return the hour
        @return Hour (0--23).
    */
    uint8_t hour() const { return hh; }

    uint8_t twelveHour() const;
    /*!
        @brief  Return whether the time is PM.
        @return 0 if the time is AM, 1 if it's PM.
    */
    uint8_t isPM() const { return hh >= 12; }
    /*!
        @brief  Return the minute.
        @return Minute (0--59).
    */
    uint8_t minute() const { return mm; }
    /*!
        @brief  Return the second.
        @return Second (0--59).
    */
    uint8_t second() const { return ss; }

    uint8_t dayOfTheWeek() const;

    /* 32-bit times as seconds since 2000-01-01. */
    uint32_t secondstime() const;

    /* 32-bit times as seconds since 1970-01-01. */
    uint32_t unixtime(void) const;

    /*!
        Format of the ISO 8601 timestamp generated by `timestamp()`. Each
        option corresponds to a `toString()` format as follows:
    */
    enum timestampOpt
    {
        TIMESTAMP_FULL, //!< `YYYY-MM-DDThh:mm:ss`
        TIMESTAMP_TIME, //!< `hh:mm:ss`
        TIMESTAMP_DATE  //!< `YYYY-MM-DD`
    };
    String timestamp(timestampOpt opt = TIMESTAMP_FULL) const;

    DateTime operator+(const TimeSpan &span) const;
    DateTime operator-(const TimeSpan &span) const;
    TimeSpan operator-(const DateTime &right) const;
    bool operator<(const DateTime &right) const;

    /*!
        @brief  Test if one DateTime is greater (later) than another.
        @warning if one or both DateTime objects are invalid, returned value is
          meaningless
        @see use `isValid()` method to check if DateTime object is valid
        @param right DateTime object to compare
        @return True if the left DateTime is later than the right one,
          false otherwise
    */
    bool operator>(const DateTime &right) const { return right < *this; }

    /*!
        @brief  Test if one DateTime is less (earlier) than or equal to another
        @warning if one or both DateTime objects are invalid, returned value is
          meaningless
        @see use `isValid()` method to check if DateTime object is valid
        @param right DateTime object to compare
        @return True if the left DateTime is earlier than or equal to the
          right one, false otherwise
    */
    bool operator<=(const DateTime &right) const { return !(*this > right); }

    /*!
        @brief  Test if one DateTime is greater (later) than or equal to another
        @warning if one or both DateTime objects are invalid, returned value is
          meaningless
        @see use `isValid()` method to check if DateTime object is valid
        @param right DateTime object to compare
        @return True if the left DateTime is later than or equal to the right
          one, false otherwise
    */
    bool operator>=(const DateTime &right) const { return !(*this < right); }
    bool operator==(const DateTime &right) const;

    /*!
        @brief  Test if two DateTime objects are not equal.
        @warning if one or both DateTime objects are invalid, returned value is
          meaningless
        @see use `isValid()` method to check if DateTime object is valid
        @param right DateTime object to compare
        @return True if the two objects are not equal, false if they are
    */
    bool operator!=(const DateTime &right) const { return !(*this == right); }

protected:
    uint8_t yOff; ///< Year offset from 2000
    uint8_t m;    ///< Month 1-12
    uint8_t d;    ///< Day 1-31
    uint8_t hh;   ///< Hours 0-23
    uint8_t mm;   ///< Minutes 0-59
    uint8_t ss;   ///< Seconds 0-59
};
#endif // _RTCLIB_H_