// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: pairing.proto

#include "pairing.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace adb {
namespace proto {
PROTOBUF_CONSTEXPR PairingPacket::PairingPacket(
    ::_pbi::ConstantInitialized) {}
struct PairingPacketDefaultTypeInternal {
  PROTOBUF_CONSTEXPR PairingPacketDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~PairingPacketDefaultTypeInternal() {}
  union {
    PairingPacket _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 PairingPacketDefaultTypeInternal _PairingPacket_default_instance_;
}  // namespace proto
}  // namespace adb
static ::_pb::Metadata file_level_metadata_pairing_2eproto[1];
static const ::_pb::EnumDescriptor* file_level_enum_descriptors_pairing_2eproto[1];
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_pairing_2eproto = nullptr;

const uint32_t TableStruct_pairing_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::adb::proto::PairingPacket, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::adb::proto::PairingPacket)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::adb::proto::_PairingPacket_default_instance_._instance,
};

const char descriptor_table_protodef_pairing_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\rpairing.proto\022\tadb.proto\"6\n\rPairingPac"
  "ket\"%\n\004Type\022\016\n\nSPAKE2_MSG\020\000\022\r\n\tPEER_INFO"
  "\020\001B-\n\035com.android.server.adb.protosB\014Pai"
  "ringProtob\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_pairing_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_pairing_2eproto = {
    false, false, 137, descriptor_table_protodef_pairing_2eproto,
    "pairing.proto",
    &descriptor_table_pairing_2eproto_once, nullptr, 0, 1,
    schemas, file_default_instances, TableStruct_pairing_2eproto::offsets,
    file_level_metadata_pairing_2eproto, file_level_enum_descriptors_pairing_2eproto,
    file_level_service_descriptors_pairing_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_pairing_2eproto_getter() {
  return &descriptor_table_pairing_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_pairing_2eproto(&descriptor_table_pairing_2eproto);
namespace adb {
namespace proto {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* PairingPacket_Type_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_pairing_2eproto);
  return file_level_enum_descriptors_pairing_2eproto[0];
}
bool PairingPacket_Type_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))
constexpr PairingPacket_Type PairingPacket::SPAKE2_MSG;
constexpr PairingPacket_Type PairingPacket::PEER_INFO;
constexpr PairingPacket_Type PairingPacket::Type_MIN;
constexpr PairingPacket_Type PairingPacket::Type_MAX;
constexpr int PairingPacket::Type_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))

// ===================================================================

class PairingPacket::_Internal {
 public:
};

PairingPacket::PairingPacket(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase(arena, is_message_owned) {
  // @@protoc_insertion_point(arena_constructor:adb.proto.PairingPacket)
}
PairingPacket::PairingPacket(const PairingPacket& from)
  : ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase() {
  PairingPacket* const _this = this; (void)_this;
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:adb.proto.PairingPacket)
}





const ::PROTOBUF_NAMESPACE_ID::Message::ClassData PairingPacket::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::CopyImpl,
    ::PROTOBUF_NAMESPACE_ID::internal::ZeroFieldsBase::MergeImpl,
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*PairingPacket::GetClassData() const { return &_class_data_; }







::PROTOBUF_NAMESPACE_ID::Metadata PairingPacket::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_pairing_2eproto_getter, &descriptor_table_pairing_2eproto_once,
      file_level_metadata_pairing_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace proto
}  // namespace adb
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::adb::proto::PairingPacket*
Arena::CreateMaybeMessage< ::adb::proto::PairingPacket >(Arena* arena) {
  return Arena::CreateMessageInternal< ::adb::proto::PairingPacket >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
