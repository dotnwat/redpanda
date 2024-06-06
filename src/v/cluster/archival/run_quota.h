#pragma once
#include "utils/named_type.h"

namespace archival {

/// Housekeeping job quota. One share is one segment reupload
/// or one deletion operation. The value can be negative if the
/// job overcommitted.
using run_quota_t = named_type<int32_t, struct _job_quota_tag>;

} // namespace archival
