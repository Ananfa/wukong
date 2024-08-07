// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: game.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_game_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_game_2eproto

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
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_game_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_game_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[2]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_game_2eproto;
namespace wukong {
namespace pb {
class AuthRequest;
struct AuthRequestDefaultTypeInternal;
extern AuthRequestDefaultTypeInternal _AuthRequest_default_instance_;
class BanResponse;
struct BanResponseDefaultTypeInternal;
extern BanResponseDefaultTypeInternal _BanResponse_default_instance_;
}  // namespace pb
}  // namespace wukong
PROTOBUF_NAMESPACE_OPEN
template<> ::wukong::pb::AuthRequest* Arena::CreateMaybeMessage<::wukong::pb::AuthRequest>(Arena*);
template<> ::wukong::pb::BanResponse* Arena::CreateMaybeMessage<::wukong::pb::BanResponse>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace wukong {
namespace pb {

// ===================================================================

class AuthRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.AuthRequest) */ {
 public:
  inline AuthRequest() : AuthRequest(nullptr) {}
  ~AuthRequest() override;
  explicit constexpr AuthRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  AuthRequest(const AuthRequest& from);
  AuthRequest(AuthRequest&& from) noexcept
    : AuthRequest() {
    *this = ::std::move(from);
  }

  inline AuthRequest& operator=(const AuthRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline AuthRequest& operator=(AuthRequest&& from) noexcept {
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
  static const AuthRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const AuthRequest* internal_default_instance() {
    return reinterpret_cast<const AuthRequest*>(
               &_AuthRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(AuthRequest& a, AuthRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(AuthRequest* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(AuthRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline AuthRequest* New() const final {
    return new AuthRequest();
  }

  AuthRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<AuthRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const AuthRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const AuthRequest& from);
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
  void InternalSwap(AuthRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.AuthRequest";
  }
  protected:
  explicit AuthRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
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
    kTokenFieldNumber = 2,
    kCipherFieldNumber = 3,
    kUserIdFieldNumber = 1,
    kRecvSerialFieldNumber = 4,
    kGateIdFieldNumber = 5,
  };
  // string token = 2;
  void clear_token();
  const std::string& token() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_token(ArgT0&& arg0, ArgT... args);
  std::string* mutable_token();
  PROTOBUF_MUST_USE_RESULT std::string* release_token();
  void set_allocated_token(std::string* token);
  private:
  const std::string& _internal_token() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_token(const std::string& value);
  std::string* _internal_mutable_token();
  public:

  // string cipher = 3;
  void clear_cipher();
  const std::string& cipher() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_cipher(ArgT0&& arg0, ArgT... args);
  std::string* mutable_cipher();
  PROTOBUF_MUST_USE_RESULT std::string* release_cipher();
  void set_allocated_cipher(std::string* cipher);
  private:
  const std::string& _internal_cipher() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_cipher(const std::string& value);
  std::string* _internal_mutable_cipher();
  public:

  // uint64 userId = 1;
  void clear_userid();
  ::PROTOBUF_NAMESPACE_ID::uint64 userid() const;
  void set_userid(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_userid() const;
  void _internal_set_userid(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // uint32 recvSerial = 4;
  void clear_recvserial();
  ::PROTOBUF_NAMESPACE_ID::uint32 recvserial() const;
  void set_recvserial(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_recvserial() const;
  void _internal_set_recvserial(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // uint32 gateId = 5;
  void clear_gateid();
  ::PROTOBUF_NAMESPACE_ID::uint32 gateid() const;
  void set_gateid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_gateid() const;
  void _internal_set_gateid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.AuthRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr token_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr cipher_;
  ::PROTOBUF_NAMESPACE_ID::uint64 userid_;
  ::PROTOBUF_NAMESPACE_ID::uint32 recvserial_;
  ::PROTOBUF_NAMESPACE_ID::uint32 gateid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_game_2eproto;
};
// -------------------------------------------------------------------

class BanResponse final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.BanResponse) */ {
 public:
  inline BanResponse() : BanResponse(nullptr) {}
  ~BanResponse() override;
  explicit constexpr BanResponse(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  BanResponse(const BanResponse& from);
  BanResponse(BanResponse&& from) noexcept
    : BanResponse() {
    *this = ::std::move(from);
  }

  inline BanResponse& operator=(const BanResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline BanResponse& operator=(BanResponse&& from) noexcept {
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
  static const BanResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const BanResponse* internal_default_instance() {
    return reinterpret_cast<const BanResponse*>(
               &_BanResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(BanResponse& a, BanResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(BanResponse* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(BanResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline BanResponse* New() const final {
    return new BanResponse();
  }

  BanResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<BanResponse>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const BanResponse& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const BanResponse& from);
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
  void InternalSwap(BanResponse* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.BanResponse";
  }
  protected:
  explicit BanResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena,
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
    kMsgIdFieldNumber = 1,
  };
  // uint32 msgId = 1;
  void clear_msgid();
  ::PROTOBUF_NAMESPACE_ID::uint32 msgid() const;
  void set_msgid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_msgid() const;
  void _internal_set_msgid(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:wukong.pb.BanResponse)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint32 msgid_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_game_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// AuthRequest

// uint64 userId = 1;
inline void AuthRequest::clear_userid() {
  userid_ = uint64_t{0u};
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 AuthRequest::_internal_userid() const {
  return userid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 AuthRequest::userid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.userId)
  return _internal_userid();
}
inline void AuthRequest::_internal_set_userid(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  userid_ = value;
}
inline void AuthRequest::set_userid(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_userid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.userId)
}

// string token = 2;
inline void AuthRequest::clear_token() {
  token_.ClearToEmpty();
}
inline const std::string& AuthRequest::token() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.token)
  return _internal_token();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void AuthRequest::set_token(ArgT0&& arg0, ArgT... args) {
 
 token_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.token)
}
inline std::string* AuthRequest::mutable_token() {
  std::string* _s = _internal_mutable_token();
  // @@protoc_insertion_point(field_mutable:wukong.pb.AuthRequest.token)
  return _s;
}
inline const std::string& AuthRequest::_internal_token() const {
  return token_.Get();
}
inline void AuthRequest::_internal_set_token(const std::string& value) {
  
  token_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArenaForAllocation());
}
inline std::string* AuthRequest::_internal_mutable_token() {
  
  return token_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArenaForAllocation());
}
inline std::string* AuthRequest::release_token() {
  // @@protoc_insertion_point(field_release:wukong.pb.AuthRequest.token)
  return token_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArenaForAllocation());
}
inline void AuthRequest::set_allocated_token(std::string* token) {
  if (token != nullptr) {
    
  } else {
    
  }
  token_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), token,
      GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:wukong.pb.AuthRequest.token)
}

// string cipher = 3;
inline void AuthRequest::clear_cipher() {
  cipher_.ClearToEmpty();
}
inline const std::string& AuthRequest::cipher() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.cipher)
  return _internal_cipher();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void AuthRequest::set_cipher(ArgT0&& arg0, ArgT... args) {
 
 cipher_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.cipher)
}
inline std::string* AuthRequest::mutable_cipher() {
  std::string* _s = _internal_mutable_cipher();
  // @@protoc_insertion_point(field_mutable:wukong.pb.AuthRequest.cipher)
  return _s;
}
inline const std::string& AuthRequest::_internal_cipher() const {
  return cipher_.Get();
}
inline void AuthRequest::_internal_set_cipher(const std::string& value) {
  
  cipher_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArenaForAllocation());
}
inline std::string* AuthRequest::_internal_mutable_cipher() {
  
  return cipher_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArenaForAllocation());
}
inline std::string* AuthRequest::release_cipher() {
  // @@protoc_insertion_point(field_release:wukong.pb.AuthRequest.cipher)
  return cipher_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArenaForAllocation());
}
inline void AuthRequest::set_allocated_cipher(std::string* cipher) {
  if (cipher != nullptr) {
    
  } else {
    
  }
  cipher_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), cipher,
      GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:wukong.pb.AuthRequest.cipher)
}

// uint32 recvSerial = 4;
inline void AuthRequest::clear_recvserial() {
  recvserial_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 AuthRequest::_internal_recvserial() const {
  return recvserial_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 AuthRequest::recvserial() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.recvSerial)
  return _internal_recvserial();
}
inline void AuthRequest::_internal_set_recvserial(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  recvserial_ = value;
}
inline void AuthRequest::set_recvserial(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_recvserial(value);
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.recvSerial)
}

// uint32 gateId = 5;
inline void AuthRequest::clear_gateid() {
  gateid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 AuthRequest::_internal_gateid() const {
  return gateid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 AuthRequest::gateid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.gateId)
  return _internal_gateid();
}
inline void AuthRequest::_internal_set_gateid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  gateid_ = value;
}
inline void AuthRequest::set_gateid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_gateid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.gateId)
}

// -------------------------------------------------------------------

// BanResponse

// uint32 msgId = 1;
inline void BanResponse::clear_msgid() {
  msgid_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 BanResponse::_internal_msgid() const {
  return msgid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 BanResponse::msgid() const {
  // @@protoc_insertion_point(field_get:wukong.pb.BanResponse.msgId)
  return _internal_msgid();
}
inline void BanResponse::_internal_set_msgid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  msgid_ = value;
}
inline void BanResponse::set_msgid(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_msgid(value);
  // @@protoc_insertion_point(field_set:wukong.pb.BanResponse.msgId)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace pb
}  // namespace wukong

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_game_2eproto
