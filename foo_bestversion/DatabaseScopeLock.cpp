#include "DatabaseScopeLock.h"

namespace bestversion {

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

} // namespace bestversion
