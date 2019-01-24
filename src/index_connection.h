#include "index.h"

namespace vroom {

class index_connection : public index {

public:
  index_connection(
      SEXP in,
      const char delim,
      const char quote,
      const bool trim_ws,
      const bool escape_double,
      const bool has_header,
      const size_t skip,
      const size_t chunk_size);

  ~index_connection() { unlink(filename_.c_str()); }
};

} // namespace vroom