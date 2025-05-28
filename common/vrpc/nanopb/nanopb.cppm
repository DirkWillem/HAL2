module;

#include <pb.h>
#include <pb_common.h>
#include <pb_decode.h>
#include <pb_encode.h>

export module nanopb;

export {
  using ::pb_byte_t;
  using ::pb_size_t;
  using ::pb_field_iter_t;

  using ::pb_istream_from_buffer;
  using ::pb_decode;

  using ::pb_ostream_from_buffer;
  using ::pb_encode;

  using ::pb_field_iter_begin;
  using ::pb_field_iter_begin_const;
  using ::pb_field_iter_next;

  namespace nanopb {
  template<typename T>
  using MsgDescriptor = MessageDescriptor<T>;
  }
}

