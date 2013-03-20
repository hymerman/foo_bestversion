#include "DatabaseScopeLock.h"

namespace lastfmgrabber {

//------------------------------------------------------------------------------

DatabaseScopeLock::DatabaseScopeLock()
	: db()
{
	db->database_lock();
}

//------------------------------------------------------------------------------

DatabaseScopeLock::~DatabaseScopeLock()
{
	db->database_unlock();
}

//------------------------------------------------------------------------------

} // namespace lastfmgrabber
