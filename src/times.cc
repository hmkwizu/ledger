/*
 * Copyright (c) 2003-2009, John Wiegley.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of New Artisans LLC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <system.hh>

#include "times.h"

namespace ledger {

int                   start_of_week          = 0;
optional<std::string> input_date_format;
std::string	      output_datetime_format = "%Y-%m-%d %H:%M:%S";
std::string	      output_date_format     = "%Y-%m-%d";

namespace {
  const char * formats[] = {
    "%y/%m/%d",
    "%Y/%m/%d",
    "%m/%d",
    "%Y/%m",
    "%y.%m.%d",
    "%Y.%m.%d",
    "%m.%d",
    "%Y.%m",
    "%y-%m-%d",
    "%Y-%m-%d",
    "%m-%d",
    "%Y-%m",
    "%a",
    "%A",
    "%b",
    "%B",
    "%Y",
    NULL
  };

  bool parse_date_mask(const char * date_str, std::tm& result)
  {
    if (input_date_format) {
      std::memset(&result, -1, sizeof(std::tm));
      if (strptime(date_str, input_date_format->c_str(), &result))
	return true;
    }
    for (const char ** f = formats; *f; f++) {
      std::memset(&result, -1, sizeof(std::tm));
      if (strptime(date_str, *f, &result))
	return true;
    }
    return false;
  }

  bool quick_parse_date(const char * date_str, std::tm& result, const int year)
  {
    if (! parse_date_mask(date_str, result))
      return false;

    result.tm_hour = 0;
    result.tm_min  = 0;
    result.tm_sec  = 0;

    if (result.tm_mday == -1)
      result.tm_mday = 1;

    if (result.tm_mon == -1) {
      result.tm_mon = 0;

      if (result.tm_mday > (CURRENT_DATE().day() - 1))
	result.tm_mon = 11;
    }

    if (result.tm_year == -1) {
      result.tm_year = (year == -1 ? int(CURRENT_DATE().year()) : year) - 1900;

      if (year == -1 && result.tm_mon > (CURRENT_DATE().month() - 1))
	result.tm_year--;
    }

    return true;
  }
}

date_time::weekdays string_to_day_of_week(const std::string& str)
{
  if (str == _("sun") || str == _("sunday") || str == "0")
    return gregorian::Sunday;
  else if (str == _("mon") || str == _("monday") || str == "1")
    return gregorian::Monday;
  else if (str == _("tue") || str == _("tuesday") || str == "2")
    return gregorian::Tuesday;
  else if (str == _("wed") || str == _("wednesday") || str == "3")
    return gregorian::Wednesday;
  else if (str == _("thu") || str == _("thursday") || str == "4")
    return gregorian::Thursday;
  else if (str == _("fri") || str == _("friday") || str == "5")
    return gregorian::Friday;
  else if (str == _("sat") || str == _("saturday") || str == "6")
    return gregorian::Saturday;

  assert(false);
  return gregorian::Sunday;
}
  
datetime_t parse_datetime(const char * str, int)
{
  std::tm when;
  std::memset(&when, -1, sizeof(std::tm));
  if (strptime(str, "%Y/%m/%d %H:%M:%S", &when))
    return posix_time::ptime_from_tm(when);
  else
    return datetime_t();
}

date_t parse_date(const char * str, int current_year)
{
  std::tm when;
  quick_parse_date(str, when, current_year);
  return gregorian::date_from_tm(when);
}

date_t date_interval_t::add_duration(const date_t&     date,
				     const duration_t& duration)
{
  if (duration.type() == typeid(gregorian::days))
    return date + boost::get<gregorian::days>(duration);
  else if (duration.type() == typeid(gregorian::weeks))
    return date + boost::get<gregorian::weeks>(duration);
  else if (duration.type() == typeid(gregorian::months))
    return date + boost::get<gregorian::months>(duration);
  else
    assert(duration.type() == typeid(gregorian::years));
  return date + boost::get<gregorian::years>(duration);
}

date_t date_interval_t::subtract_duration(const date_t&     date,
					  const duration_t& duration)
{
  if (duration.type() == typeid(gregorian::days))
    return date - boost::get<gregorian::days>(duration);
  else if (duration.type() == typeid(gregorian::weeks))
    return date - boost::get<gregorian::weeks>(duration);
  else if (duration.type() == typeid(gregorian::months))
    return date - boost::get<gregorian::months>(duration);
  else
    assert(duration.type() == typeid(gregorian::years));
  return date - boost::get<gregorian::years>(duration);
}

std::ostream& operator<<(std::ostream& out,
			 const date_interval_t::duration_t& duration)
{
  if (duration.type() == typeid(gregorian::days))
    out << boost::get<gregorian::days>(duration).days()
	<< " days";
  else if (duration.type() == typeid(gregorian::weeks))
    out << (boost::get<gregorian::weeks>(duration).days() / 7)
	<< " weeks";
  else if (duration.type() == typeid(gregorian::months))
    out << boost::get<gregorian::months>(duration).number_of_months()
	<< " months";
  else {
    assert(duration.type() == typeid(gregorian::years));
    out << boost::get<gregorian::years>(duration).number_of_years()
	<< " years";
  }
  return out;
}

bool date_interval_t::find_period(const date_t&     date,
				  date_interval_t * last_interval)
{
  if (end && date > *end)
    return false;

  if (! start) {
    if (duration) {
      // The interval object has not been seeded with a start date yet, so
      // find the nearest period before on on date which fits, if possible.
      //
      // Find an efficient starting point for the upcoming while loop.  We
      // want a date early enough that the range will be correct, but late
      // enough that we don't spend hundreds of thousands of loops skipping
      // through time.
      if (duration->type() == typeid(gregorian::months) ||
	  duration->type() == typeid(gregorian::weeks)) {
	start = date_t(date.year(), gregorian::Jan, 1);
      } else {
	start = date_t(date - gregorian::days(400));

	if (duration->type() == typeid(gregorian::weeks)) {
	  // Move it to a Sunday
	  while (start->day_of_week() != start_of_week)
	    *start += gregorian::days(1);
	}
      }
    }
  }

  if (date < *start)
    return false;

  // If there is no duration, then if we've reached here the date falls
  // between begin and end.
  if (! duration) {
    if (! start && ! end)
      throw_(date_error,
	     _("Invalid date interval: neither start, nor end, nor duration"));
    return true;
  }

  if (! end_of_duration)
    end_of_duration = add_duration(*start, *duration);

  if (! skip_duration)
    skip_duration = duration;

  if (! next)
    next = add_duration(*start, *skip_duration);

  if (date < *end_of_duration)
    return true;

  // If we've reached here, it means the date does not fall into the current
  // interval, so we must seek another interval that does match -- unless we
  // pass by date in so doing, which means we shouldn't alter the current
  // period of the interval at all.

  date_t scan        = *next;
  date_t end_of_scan = add_duration(scan, *duration);

  while (date >= scan && (! end || scan < *end)) {
    if (date < end_of_scan) {
      if (last_interval) {
	last_interval->start	     = start;
	last_interval->next	     = next;
	last_interval->end_of_duration = end_of_duration;
      }
      start	      = scan;
      end_of_duration = end_of_scan;
      next	      = none;
      return true;
    }
    scan = add_duration(scan, *skip_duration);
    end_of_scan = add_duration(scan, *duration);
  }

  return false;
}

date_interval_t& date_interval_t::operator++()
{
  if (! start)
    throw_(date_error, _("Cannot increment an unstarted date interval"));

  if (! skip_duration) {
    if (duration)
      skip_duration = duration;
    else
      throw_(date_error,
	     _("Cannot increment a date interval without a duration"));
  }

  *start = add_duration(*start, *skip_duration);

  if (end && *start >= *end)
    start = none;
  else
    end_of_duration = add_duration(*start, *duration);

  return *this;
}

namespace {
  void parse_inclusion_specifier(const string& word,
				 date_t *      begin,
				 date_t *      end)
  {
    struct std::tm when;

    if (! parse_date_mask(word.c_str(), when))
      throw_(date_error, _("Could not parse date mask: %1") << word);

    when.tm_hour   = 0;
    when.tm_min	   = 0;
    when.tm_sec	   = 0;
    when.tm_isdst  = -1;

    bool saw_year = true;
    bool saw_mon  = true;
    bool saw_day  = true;

    if (when.tm_year == -1) {
      when.tm_year = CURRENT_DATE().year() - 1900;
      saw_year = false;
    }
    if (when.tm_mon == -1) {
      when.tm_mon = 0;
      saw_mon = false;
    } else {
      saw_year = false;		// don't increment by year if month used
    }
    if (when.tm_mday == -1) {
      when.tm_mday = 1;
      saw_day = false;
    } else {
      saw_mon  = false;		// don't increment by month if day used
      saw_year = false;		// don't increment by year if day used
    }

    if (begin) {
      *begin = gregorian::date_from_tm(when);

      if (end) {
	if (saw_year)
	  *end = *begin + gregorian::years(1);
	else if (saw_mon)
	  *end = *begin + gregorian::months(1);
	else if (saw_day)
	  *end = *begin + gregorian::days(1);
      }
    }
    else if (end) {
      *end = gregorian::date_from_tm(when);
    }
  }

  inline void read_lower_word(std::istream& in, string& word) {
    in >> word;
    for (int i = 0, l = word.length(); i < l; i++)
      word[i] = static_cast<char>(std::tolower(word[i]));
  }

  void parse_date_words(std::istream&	 in,
			string&		 word,
			date_interval_t& interval,
			bool             look_for_start = true,
			bool             look_for_end   = true)
  {
    string type;

    if (word == _("this") || word == _("last") || word == _("next")) {
      type = word;
      if (! in.eof())
	read_lower_word(in, word);
      else
	word = _("month");
    } else {
      type = _("this");
    }

    date_t start = CURRENT_DATE();
    date_t end;
    bool   parse_specifier = false;

    date_interval_t::duration_t duration;

    assert(look_for_start || look_for_end);

    if (word == _("year")) {
      duration = gregorian::years(1);
      start    = gregorian::date(start.year(), 1, 1);
    }
    else if (word == _("month")) {
      duration = gregorian::months(1);
      start    = gregorian::date(start.year(), start.month(), 1);
    }
    else if (word == _("today") || word == _("day")) {
      duration = gregorian::days(1);
    }
    else {
      parse_specifier = true;
    }
    end = date_interval_t::add_duration(start, duration);

    if (parse_specifier)
      parse_inclusion_specifier(word, &start, &end);

    if (type == _("last")) {
      start = date_interval_t::subtract_duration(start, duration);
      end   = date_interval_t::subtract_duration(end, duration);
    }
    else if (type == _("next")) {
      start = date_interval_t::add_duration(start, duration);
      end   = date_interval_t::add_duration(end, duration);
    }

    if (look_for_start) interval.start = start;
    if (look_for_end)   interval.end = end;
  }
}

void date_interval_t::parse(std::istream& in)
{
  string word;

  while (! in.eof()) {
    read_lower_word(in, word);
    if (word == _("every")) {
      read_lower_word(in, word);
      if (std::isdigit(word[0])) {
	int quantity = lexical_cast<int>(word);
	read_lower_word(in, word);
	if (word == _("days"))
	  duration = gregorian::days(quantity);
	else if (word == _("weeks"))
	  duration = gregorian::weeks(quantity);
	else if (word == _("months"))
	  duration = gregorian::months(quantity);
	else if (word == _("quarters"))
	  duration = gregorian::months(3 * quantity);
	else if (word == _("years"))
	  duration = gregorian::years(quantity);
      }
      else if (word == _("day"))
	duration = gregorian::days(1);
      else if (word == _("week"))
	duration = gregorian::weeks(1);
      else if (word == _("month"))
	duration = gregorian::months(1);
      else if (word == _("quarter"))
	duration = gregorian::months(3);
      else if (word == _("year"))
	duration = gregorian::years(1);
    }
    else if (word == _("daily"))
      duration = gregorian::days(1);
    else if (word == _("weekly"))
      duration = gregorian::weeks(1);
    else if (word == _("biweekly"))
      duration = gregorian::weeks(2);
    else if (word == _("monthly"))
      duration = gregorian::months(1);
    else if (word == _("bimonthly"))
      duration = gregorian::months(2);
    else if (word == _("quarterly"))
      duration = gregorian::months(3);
    else if (word == _("yearly"))
      duration = gregorian::years(1);
    else if (word == _("this") || word == _("last") || word == _("next") ||
	     word == _("today")) {
      parse_date_words(in, word, *this);
    }
    else if (word == _("in")) {
      read_lower_word(in, word);
      parse_date_words(in, word, *this);
    }
    else if (word == _("from") || word == _("since")) {
      read_lower_word(in, word);
      parse_date_words(in, word, *this, true, false);
    }
    else if (word == _("to") || word == _("until")) {
      read_lower_word(in, word);
      parse_date_words(in, word, *this, false, true);
    }
    else {
      date_t b, e;
      parse_inclusion_specifier(word, &b, &e);
      start = b;
      end   = e;
    }
  }
}

} // namespace ledger
