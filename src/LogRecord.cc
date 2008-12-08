//////////////////////////////////////////////////////////////////////////////
// LogRecord.cc
//
// Contact: Ray Plante
// 
//////////////////////////////////////////////////////////////////////////////

#include "lsst/pex/logging/LogRecord.h"
#include "lsst/pex/exceptions.h"
#include "lsst/daf/base/DateTime.h"

#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <time.h>

namespace lsst {
namespace pex {
namespace logging {

using boost::format;
using lsst::daf::base::DateTime;
namespace pexExcept = lsst::pex::exceptions;


/**
 * Create a log record to be sent to a given log.  
 * @param verbosity  the loudness of the record.  If this value is 
 *                     greater than or equal to the Log's verbosity 
 *                     threshold, the message will be recorded.
 */
LogRecord::LogRecord(int threshold, int verbosity)
    : _send(threshold <= verbosity), _vol(verbosity), _data(new PropertySet())
{ 
    _init();
}

/**
 * Create a log record to be sent to a given log.  The current time is 
 * recorded and set as the DATE property.
 * @param threshold  the verbosity threshold that determines if a message
 *                     is printed.
 * @param verbosity  the loudness of the record.  If this value is 
 *                     greater than or equal to the given verbosity 
 *                     threshold, the message will be recorded.
 * @param preamble   an ordered set of properties that constitute the 
 *                     preamble of this message.  This should not include
 *                     the current time.  
 */
LogRecord::LogRecord(int threshold, int verbosity, const PropertySet& preamble) 
    : _send(threshold <= verbosity), _vol(verbosity), _data()
{
    if (_send) {
        _data = preamble.deepCopy();
    }
    else {
        _data = boost::shared_ptr<PropertySet>(new PropertySet());
    }
    _init();
}

/**
 * delete this log record
 */
LogRecord::~LogRecord() { }

void LogRecord::setTimestamp() {
    struct timeval tv;      
    struct timezone tz;     
    gettimeofday(&tv,&tz);
    // _tv.tv_sec = seconds since the epoch
    // _tv.tv_usec = microseconds since tv.tv_sec

    long long nsec = static_cast<long long>(tv.tv_sec) * 1000000000L;
    nsec += tv.tv_usec * 1000L;
    _data->set(LSST_LP_TIMESTAMP, lsst::daf::base::DateTime(nsec));
}

void LogRecord::setDate() {
    if (! _send) return;
    if (! data().exists(LSST_LP_TIMESTAMP)) setTimestamp();

    char datestr[40];
    struct timeval tv = _data->get<DateTime>(LSST_LP_TIMESTAMP).timeval();

    struct tm timeinfo;
    time_t secs = (time_t) tv.tv_sec;
    gmtime_r(&secs, &timeinfo);

    if ( 0 == strftime(datestr,39,"%Y-%m-%dT%H:%M:%S.", &timeinfo) ) {
        throw LSST_EXCEPT(pexExcept::RuntimeErrorException, 
                          "Failed to format time successfully");
    }
    
    string fulldate(str(format("%s%d") % string(datestr) % tv.tv_usec));
    data().add(LSST_LP_DATE, fulldate);
}

size_t LogRecord::countParamValues() const {
    size_t sum = 0;
    std::vector<std::string> names = _data->names(false);
    std::vector<std::string>::iterator it;
    for(it = names.begin(); it != names.end(); ++it) {
        sum += _data->valueCount(*it);
    }
    return sum;
}

void LogRecord::addProperties(const PropertySet& props) {
    PropertySet::Ptr temp(props.deepCopy());
    if (temp->exists("LEVEL")) temp->remove("LEVEL");
    if (temp->exists("LOG")) temp->remove("LOG");
    if (temp->exists("TIMESTAMP")) temp->remove("TIMESTAMP");
    if (temp->exists("DATE")) temp->remove("DATE");
    data().combine(temp);
}

}}} // end lsst::pex::logging

