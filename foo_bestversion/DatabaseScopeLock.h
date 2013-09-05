#pragma once

#include "FoobarSDKWrapper.h"

namespace bestversion {

class DatabaseScopeLock
{
public:
	DatabaseScopeLock();
	~DatabaseScopeLock();

private:
	// Non-copyable.
	DatabaseScopeLock(const DatabaseScopeLock&);
	DatabaseScopeLock& operator=(const DatabaseScopeLock&);

	static_api_ptr_t<metadb> db;
};

} // namespace bestversion
