#pragma once

#include "FoobarSDKWrapper.h"

namespace lastfmgrabber {

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

} // namespace lastfmgrabber
