// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: scene_service.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_scene_5fservice_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_scene_5fservice_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3017000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3017003 < PROTOBUF_MIN_PROTOC_VERSION
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
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
#include "corpc_option.pb.h"
#include "common.pb.h"
#include "inner_common.pb.h"
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_scene_5fservice_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_scene_5fservice_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[3]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_scene_5fservice_2eproto;
namespace wukong {
namespace pb {
class EnterSceneRequest;
struct EnterSceneRequestDefaultTypeInternal;
extern EnterSceneRequestDefaultTypeInternal _EnterSceneRequest_default_instance_;
class LoadSceneRequest;
struct LoadSceneRequestDefaultTypeInternal;
extern LoadSceneRequestDefaultTypeInternal _LoadSceneRequest_default_instance_;
class LoadSceneResponse;
struct LoadSceneResponseDefaultTypeInternal;
extern LoadSceneResponseDefaultTypeInternal _LoadSceneResponse_default_instance_;
}  // namespace pb
}  // namespace wukong
PROTOBUF_NAMESPACE_OPEN
template<> ::wukong::pb::EnterSceneRequest* Arena::CreateMaybeMessage<::wukong::pb::EnterSceneRequest>(Arena*);
template<> ::wukong::pb::LoadSceneRequest* Arena::CreateMaybeMessage<::wukong::pb::LoadSceneRequest>(Arena*);
template<> ::wukong::pb::LoadSceneResponse* Arena::CreateMaybeMessage<::wukong::pb::LoadSceneResponse>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace wukong {
namespace pb {

// ===================================================================

class LoadSceneRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.LoadSceneRequest) */ {
 public:
  inline LoadSceneRequest() : LoadSceneRequest(nullptr) {}
  ~LoadSceneRequest() override;
  explicit constexpr LoadSceneRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  LoadSceneRequest(const LoadSceneRequest& from);
  LoadSceneRequest(LoadSceneRequest&& from) noexcept
    : LoadSceneRequest() {
    *this = ::std::move(from);
  }

  inline LoadSceneRequest& operator=(const LoadSceneRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline LoadSceneRequest& operator=(LoadSceneRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const LoadSceneRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const LoadSceneRequest* internal_default_instance() {
    return reinterpret_cast<const LoadSceneRequest*>(
               &_LoadSceneRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(LoadSceneRequest& a, LoadSceneRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(LoadSceneRequest* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(LoadSceneRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline LoadSceneRequest* New() const final {
    return new LoadSceneRequest();
  }

  LoadSceneRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<LoadSceneRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const LoadSceneRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const LoadSceneRequest& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to, const ::PROTOBUF_NAMESPACE_ID::Message&from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(LoadSceneRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.LoadSceneRequest";
  }
  protected:
  explicit LoadSceneRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kDefIdFieldNumber = 1,
    kSceneIdFieldNumber = 2,
  };
  // uint32 defId = 1;
  void clear_defid();
  ::PROTOBUF_NAMESPACE_ID::uint32 defid() const;
  void set_defid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_defid() const;
  void _internal_set_defid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 sceneId = 2;
  void clear_sceneid();
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid() const;
  void set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_sceneid() const;
  void _internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.LoadSceneRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 defid_;
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_scene_5fservice_2eproto;
};
// -------------------------------------------------------------------

class LoadSceneResponse final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.LoadSceneResponse) */ {
 public:
  inline LoadSceneResponse() : LoadSceneResponse(nullptr) {}
  ~LoadSceneResponse() override;
  explicit constexpr LoadSceneResponse(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  LoadSceneResponse(const LoadSceneResponse& from);
  LoadSceneResponse(LoadSceneResponse&& from) noexcept
    : LoadSceneResponse() {
    *this = ::std::move(from);
  }

  inline LoadSceneResponse& operator=(const LoadSceneResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline LoadSceneResponse& operator=(LoadSceneResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const LoadSceneResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const LoadSceneResponse* internal_default_instance() {
    return reinterpret_cast<const LoadSceneResponse*>(
               &_LoadSceneResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(LoadSceneResponse& a, LoadSceneResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(LoadSceneResponse* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(LoadSceneResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline LoadSceneResponse* New() const final {
    return new LoadSceneResponse();
  }

  LoadSceneResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<LoadSceneResponse>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const LoadSceneResponse& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const LoadSceneResponse& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to, const ::PROTOBUF_NAMESPACE_ID::Message&from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(LoadSceneResponse* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.LoadSceneResponse";
  }
  protected:
  explicit LoadSceneResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kErrCodeFieldNumber = 1,
    kSceneIdFieldNumber = 2,
  };
  // uint32 errCode = 1;
  void clear_errcode();
  ::PROTOBUF_NAMESPACE_ID::uint32 errcode() const;
  void set_errcode(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_errcode() const;
  void _internal_set_errcode(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 sceneId = 2;
  void clear_sceneid();
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid() const;
  void set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_sceneid() const;
  void _internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.LoadSceneResponse)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 errcode_;
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_scene_5fservice_2eproto;
};
// -------------------------------------------------------------------

class EnterSceneRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.EnterSceneRequest) */ {
 public:
  inline EnterSceneRequest() : EnterSceneRequest(nullptr) {}
  ~EnterSceneRequest() override;
  explicit constexpr EnterSceneRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  EnterSceneRequest(const EnterSceneRequest& from);
  EnterSceneRequest(EnterSceneRequest&& from) noexcept
    : EnterSceneRequest() {
    *this = ::std::move(from);
  }

  inline EnterSceneRequest& operator=(const EnterSceneRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline EnterSceneRequest& operator=(EnterSceneRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const EnterSceneRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const EnterSceneRequest* internal_default_instance() {
    return reinterpret_cast<const EnterSceneRequest*>(
               &_EnterSceneRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(EnterSceneRequest& a, EnterSceneRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(EnterSceneRequest* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(EnterSceneRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline EnterSceneRequest* New() const final {
    return new EnterSceneRequest();
  }

  EnterSceneRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<EnterSceneRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const EnterSceneRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const EnterSceneRequest& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to, const ::PROTOBUF_NAMESPACE_ID::Message&from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(EnterSceneRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.EnterSceneRequest";
  }
  protected:
  explicit EnterSceneRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kRoleIdFieldNumber = 1,
    kSceneIdFieldNumber = 2,
  };
  // uint32 roleId = 1;
  void clear_roleid();
  ::PROTOBUF_NAMESPACE_ID::uint32 roleid() const;
  void set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_roleid() const;
  void _internal_set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 sceneId = 2;
  void clear_sceneid();
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid() const;
  void set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_sceneid() const;
  void _internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.EnterSceneRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 roleid_;
  ::PROTOBUF_NAMESPACE_ID::uint32 sceneid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_scene_5fservice_2eproto;
};
// ===================================================================

class SceneService_Stub;

class SceneService : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline SceneService() {};
 public:
  virtual ~SceneService();

  typedef SceneService_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::OnlineCounts* response,
                       ::google::protobuf::Closure* done);
  virtual void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::LoadSceneRequest* request,
                       ::wukong::pb::LoadSceneResponse* response,
                       ::google::protobuf::Closure* done);
  virtual void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::EnterSceneRequest* request,
                       ::corpc::Void* response,
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
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SceneService);
};

class SceneService_Stub : public SceneService {
 public:
  SceneService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
  SceneService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel,
                   ::PROTOBUF_NAMESPACE_ID::Service::ChannelOwnership ownership);
  ~SceneService_Stub();

  inline ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel() { return channel_; }

  // implements SceneService ------------------------------------------

  void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::OnlineCounts* response,
                       ::google::protobuf::Closure* done);
  void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::LoadSceneRequest* request,
                       ::wukong::pb::LoadSceneResponse* response,
                       ::google::protobuf::Closure* done);
  void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::EnterSceneRequest* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
 private:
  ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SceneService_Stub);
};


// -------------------------------------------------------------------

class InnerSceneService_Stub;

class InnerSceneService : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline InnerSceneService() {};
 public:
  virtual ~InnerSceneService();

  typedef InnerSceneService_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);
  virtual void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::LoadSceneRequest* request,
                       ::wukong::pb::LoadSceneResponse* response,
                       ::google::protobuf::Closure* done);
  virtual void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::EnterSceneRequest* request,
                       ::corpc::Void* response,
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
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(InnerSceneService);
};

class InnerSceneService_Stub : public InnerSceneService {
 public:
  InnerSceneService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
  InnerSceneService_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel,
                   ::PROTOBUF_NAMESPACE_ID::Service::ChannelOwnership ownership);
  ~InnerSceneService_Stub();

  inline ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel() { return channel_; }

  // implements InnerSceneService ------------------------------------------

  void shutdown(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
  void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::corpc::Void* request,
                       ::wukong::pb::Uint32Value* response,
                       ::google::protobuf::Closure* done);
  void loadScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::LoadSceneRequest* request,
                       ::wukong::pb::LoadSceneResponse* response,
                       ::google::protobuf::Closure* done);
  void enterScene(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::wukong::pb::EnterSceneRequest* request,
                       ::corpc::Void* response,
                       ::google::protobuf::Closure* done);
 private:
  ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(InnerSceneService_Stub);
};


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// LoadSceneRequest

// uint32 defId = 1;
inline void LoadSceneRequest::clear_defid() {
  defid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneRequest::_internal_defid() const {
  return defid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneRequest::defid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.LoadSceneRequest.defId)
  return _internal_defid();
}
inline void LoadSceneRequest::_internal_set_defid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  defid_ = value;
}
inline void LoadSceneRequest::set_defid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_defid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.LoadSceneRequest.defId)
}

// uint32 sceneId = 2;
inline void LoadSceneRequest::clear_sceneid() {
  sceneid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneRequest::_internal_sceneid() const {
  return sceneid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneRequest::sceneid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.LoadSceneRequest.sceneId)
  return _internal_sceneid();
}
inline void LoadSceneRequest::_internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  sceneid_ = value;
}
inline void LoadSceneRequest::set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_sceneid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.LoadSceneRequest.sceneId)
}

// -------------------------------------------------------------------

// LoadSceneResponse

// uint32 errCode = 1;
inline void LoadSceneResponse::clear_errcode() {
  errcode_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneResponse::_internal_errcode() const {
  return errcode_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneResponse::errcode() const {
  // @@protoc_insertion_point(field_get:wukong.pb.LoadSceneResponse.errCode)
  return _internal_errcode();
}
inline void LoadSceneResponse::_internal_set_errcode(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  errcode_ = value;
}
inline void LoadSceneResponse::set_errcode(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_errcode(value);
  // @@protoc_insertion_point(field_set:wukong.pb.LoadSceneResponse.errCode)
}

// uint32 sceneId = 2;
inline void LoadSceneResponse::clear_sceneid() {
  sceneid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneResponse::_internal_sceneid() const {
  return sceneid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 LoadSceneResponse::sceneid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.LoadSceneResponse.sceneId)
  return _internal_sceneid();
}
inline void LoadSceneResponse::_internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  sceneid_ = value;
}
inline void LoadSceneResponse::set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_sceneid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.LoadSceneResponse.sceneId)
}

// -------------------------------------------------------------------

// EnterSceneRequest

// uint32 roleId = 1;
inline void EnterSceneRequest::clear_roleid() {
  roleid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 EnterSceneRequest::_internal_roleid() const {
  return roleid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 EnterSceneRequest::roleid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.EnterSceneRequest.roleId)
  return _internal_roleid();
}
inline void EnterSceneRequest::_internal_set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  roleid_ = value;
}
inline void EnterSceneRequest::set_roleid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_roleid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.EnterSceneRequest.roleId)
}

// uint32 sceneId = 2;
inline void EnterSceneRequest::clear_sceneid() {
  sceneid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 EnterSceneRequest::_internal_sceneid() const {
  return sceneid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 EnterSceneRequest::sceneid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.EnterSceneRequest.sceneId)
  return _internal_sceneid();
}
inline void EnterSceneRequest::_internal_set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  sceneid_ = value;
}
inline void EnterSceneRequest::set_sceneid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_sceneid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.EnterSceneRequest.sceneId)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace pb
}  // namespace wukong

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_scene_5fservice_2eproto