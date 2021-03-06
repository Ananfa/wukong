// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: lobby_service.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_lobby_5fservice_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_lobby_5fservice_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3012000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3012003 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
#include "corpc_option.pb.h"
#include "common.pb.h"
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_lobby_5fservice_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_lobby_5fservice_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_lobby_5fservice_2eproto;
namespace wukong {
namespace pb {
class InitRoleRequest;
class InitRoleRequestDefaultTypeInternal;
extern InitRoleRequestDefaultTypeInternal _InitRoleRequest_default_instance_;
}  // namespace pb
}  // namespace wukong
PROTOBUF_NAMESPACE_OPEN
template<> ::wukong::pb::InitRoleRequest* Arena::CreateMaybeMessage<::wukong::pb::InitRoleRequest>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace wukong {
namespace pb {

// ===================================================================

class InitRoleRequest PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.InitRoleRequest) */ {
 public:
  inline InitRoleRequest() : InitRoleRequest(nullptr) {};
  virtual ~InitRoleRequest();

  InitRoleRequest(const InitRoleRequest& from);
  InitRoleRequest(InitRoleRequest&& from) noexcept
    : InitRoleRequest() {
    *this = ::std::move(from);
  }

  inline InitRoleRequest& operator=(const InitRoleRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline InitRoleRequest& operator=(InitRoleRequest&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const InitRoleRequest& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const InitRoleRequest* internal_default_instance() {
    return reinterpret_cast<const InitRoleRequest*>(
               &_InitRoleRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(InitRoleRequest& a, InitRoleRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(InitRoleRequest* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(InitRoleRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline InitRoleRequest* New() const final {
    return CreateMaybeMessage<InitRoleRequest>(nullptr);
  }

  InitRoleRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<InitRoleRequest>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const InitRoleRequest& from);
  void MergeFrom(const InitRoleRequest& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(InitRoleRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.InitRoleRequest";
  }
  protected:
  explicit InitRoleRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_lobby_5fservice_2eproto);
    return ::descriptor_table_lobby_5fservice_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kUserIdFieldNumber = 1,
    kRoleIdFieldNumber = 2,
    kGatewayIdFieldNumber = 3,
  };
  // uint32 userId = 1;
  void clear_userid();
  ::PROTOBUF_NAMESPACE_ID::uint32 userid() const;
  void set_userid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_userid() const;
  void _internal_set_userid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 roleId = 2;
  void clear_roleid();
  ::PROTOBUF_NAMESPACE_ID::uint32 roleid() const;
  void set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_roleid() const;
  void _internal_set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 gatewayId = 3;
  void clear_gatewayid();
  ::PROTOBUF_NAMESPACE_ID::uint32 gatewayid() const;
  void set_gatewayid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_gatewayid() const;
  void _internal_set_gatewayid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.InitRoleRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 userid_;
  ::PROTOBUF_NAMESPACE_ID::uint32 roleid_;
  ::PROTOBUF_NAMESPACE_ID::uint32 gatewayid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_lobby_5fservice_2eproto;
};
// ===================================================================

class LobbyService_Stub;

class LobbyService : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline LobbyService() {};
 public:
  virtual ~LobbyService();

  typedef LobbyService_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);
  virtual void initRole(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::InitRoleRequest* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                  ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                  const ::PROTOBUF_NAMESPACE_ID::Message* request,
                  ::PROTOBUF_NAMESPACE_ID::Message* response,
                  ::google::protobuf::Closure* done);
  const ::PROTOBUF_NAMESPACE_ID::Message& GetRequestPrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;
  const ::PROTOBUF_NAMESPACE_ID::Message& GetResponsePrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LobbyService);
};

class LobbyService_Stub : public LobbyService {
 public:
  LobbyService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
  LobbyService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel,
                   ::PROTOBUF_NAMESPACE_ID::Service::ChannelOwnership ownership);
  ~LobbyService_Stub();

  inline ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel() { return channel_; }

  // implements LobbyService ------------------------------------------

  void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);
  void initRole(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::InitRoleRequest* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);
 private:
  ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LobbyService_Stub);
};


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// InitRoleRequest

// uint32 userId = 1;
inline void InitRoleRequest::clear_userid() {
  userid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::_internal_userid() const {
  return userid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::userid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.InitRoleRequest.userId)
  return _internal_userid();
}
inline void InitRoleRequest::_internal_set_userid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  userid_ = value;
}
inline void InitRoleRequest::set_userid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_userid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.InitRoleRequest.userId)
}

// uint32 roleId = 2;
inline void InitRoleRequest::clear_roleid() {
  roleid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::_internal_roleid() const {
  return roleid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::roleid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.InitRoleRequest.roleId)
  return _internal_roleid();
}
inline void InitRoleRequest::_internal_set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  roleid_ = value;
}
inline void InitRoleRequest::set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_roleid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.InitRoleRequest.roleId)
}

// uint32 gatewayId = 3;
inline void InitRoleRequest::clear_gatewayid() {
  gatewayid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::_internal_gatewayid() const {
  return gatewayid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 InitRoleRequest::gatewayid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.InitRoleRequest.gatewayId)
  return _internal_gatewayid();
}
inline void InitRoleRequest::_internal_set_gatewayid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  gatewayid_ = value;
}
inline void InitRoleRequest::set_gatewayid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_gatewayid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.InitRoleRequest.gatewayId)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace pb
}  // namespace wukong

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_lobby_5fservice_2eproto
