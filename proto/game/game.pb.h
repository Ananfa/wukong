// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: game.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_game_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_game_2eproto

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
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxillaryParseTableField aux[]
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
class AuthRequestDefaultTypeInternal;
extern AuthRequestDefaultTypeInternal _AuthRequest_default_instance_;
class BanResponse;
class BanResponseDefaultTypeInternal;
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

class AuthRequest PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.AuthRequest) */ {
 public:
  inline AuthRequest() : AuthRequest(nullptr) {};
  virtual ~AuthRequest();

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
  static const AuthRequest& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
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
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(AuthRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline AuthRequest* New() const final {
    return CreateMaybeMessage<AuthRequest>(nullptr);
  }

  AuthRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<AuthRequest>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const AuthRequest& from);
  void MergeFrom(const AuthRequest& from);
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
  void InternalSwap(AuthRequest* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.AuthRequest";
  }
  protected:
  explicit AuthRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_game_2eproto);
    return ::descriptor_table_game_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kTokenFieldNumber = 2,
    kCipherFieldNumber = 3,
    kUserIdFieldNumber = 1,
    kRecvSerialFieldNumber = 4,
  };
  // string token = 2;
  void clear_token();
  const std::string& token() const;
  void set_token(const std::string& value);
  void set_token(std::string&& value);
  void set_token(const char* value);
  void set_token(const char* value, size_t size);
  std::string* mutable_token();
  std::string* release_token();
  void set_allocated_token(std::string* token);
  GOOGLE_PROTOBUF_RUNTIME_DEPRECATED("The unsafe_arena_ accessors for"
  "    string fields are deprecated and will be removed in a"
  "    future release.")
  std::string* unsafe_arena_release_token();
  GOOGLE_PROTOBUF_RUNTIME_DEPRECATED("The unsafe_arena_ accessors for"
  "    string fields are deprecated and will be removed in a"
  "    future release.")
  void unsafe_arena_set_allocated_token(
      std::string* token);
  private:
  const std::string& _internal_token() const;
  void _internal_set_token(const std::string& value);
  std::string* _internal_mutable_token();
  public:

  // string cipher = 3;
  void clear_cipher();
  const std::string& cipher() const;
  void set_cipher(const std::string& value);
  void set_cipher(std::string&& value);
  void set_cipher(const char* value);
  void set_cipher(const char* value, size_t size);
  std::string* mutable_cipher();
  std::string* release_cipher();
  void set_allocated_cipher(std::string* cipher);
  GOOGLE_PROTOBUF_RUNTIME_DEPRECATED("The unsafe_arena_ accessors for"
  "    string fields are deprecated and will be removed in a"
  "    future release.")
  std::string* unsafe_arena_release_cipher();
  GOOGLE_PROTOBUF_RUNTIME_DEPRECATED("The unsafe_arena_ accessors for"
  "    string fields are deprecated and will be removed in a"
  "    future release.")
  void unsafe_arena_set_allocated_cipher(
      std::string* cipher);
  private:
  const std::string& _internal_cipher() const;
  void _internal_set_cipher(const std::string& value);
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
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_game_2eproto;
};
// -------------------------------------------------------------------

class BanResponse PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:wukong.pb.BanResponse) */ {
 public:
  inline BanResponse() : BanResponse(nullptr) {};
  virtual ~BanResponse();

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
  static const BanResponse& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
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
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(BanResponse* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline BanResponse* New() const final {
    return CreateMaybeMessage<BanResponse>(nullptr);
  }

  BanResponse* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<BanResponse>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const BanResponse& from);
  void MergeFrom(const BanResponse& from);
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
  void InternalSwap(BanResponse* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "wukong.pb.BanResponse";
  }
  protected:
  explicit BanResponse(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_game_2eproto);
    return ::descriptor_table_game_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

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
  userid_ = PROTOBUF_ULONGLONG(0);
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
  token_.ClearToEmpty(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline const std::string& AuthRequest::token() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.token)
  return _internal_token();
}
inline void AuthRequest::set_token(const std::string& value) {
  _internal_set_token(value);
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.token)
}
inline std::string* AuthRequest::mutable_token() {
  // @@protoc_insertion_point(field_mutable:wukong.pb.AuthRequest.token)
  return _internal_mutable_token();
}
inline const std::string& AuthRequest::_internal_token() const {
  return token_.Get();
}
inline void AuthRequest::_internal_set_token(const std::string& value) {
  
  token_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void AuthRequest::set_token(std::string&& value) {
  
  token_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:wukong.pb.AuthRequest.token)
}
inline void AuthRequest::set_token(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  token_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArena());
  // @@protoc_insertion_point(field_set_char:wukong.pb.AuthRequest.token)
}
inline void AuthRequest::set_token(const char* value,
    size_t size) {
  
  token_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArena());
  // @@protoc_insertion_point(field_set_pointer:wukong.pb.AuthRequest.token)
}
inline std::string* AuthRequest::_internal_mutable_token() {
  
  return token_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* AuthRequest::release_token() {
  // @@protoc_insertion_point(field_release:wukong.pb.AuthRequest.token)
  return token_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void AuthRequest::set_allocated_token(std::string* token) {
  if (token != nullptr) {
    
  } else {
    
  }
  token_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), token,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:wukong.pb.AuthRequest.token)
}
inline std::string* AuthRequest::unsafe_arena_release_token() {
  // @@protoc_insertion_point(field_unsafe_arena_release:wukong.pb.AuthRequest.token)
  GOOGLE_DCHECK(GetArena() != nullptr);
  
  return token_.UnsafeArenaRelease(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArena());
}
inline void AuthRequest::unsafe_arena_set_allocated_token(
    std::string* token) {
  GOOGLE_DCHECK(GetArena() != nullptr);
  if (token != nullptr) {
    
  } else {
    
  }
  token_.UnsafeArenaSetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      token, GetArena());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:wukong.pb.AuthRequest.token)
}

// string cipher = 3;
inline void AuthRequest::clear_cipher() {
  cipher_.ClearToEmpty(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline const std::string& AuthRequest::cipher() const {
  // @@protoc_insertion_point(field_get:wukong.pb.AuthRequest.cipher)
  return _internal_cipher();
}
inline void AuthRequest::set_cipher(const std::string& value) {
  _internal_set_cipher(value);
  // @@protoc_insertion_point(field_set:wukong.pb.AuthRequest.cipher)
}
inline std::string* AuthRequest::mutable_cipher() {
  // @@protoc_insertion_point(field_mutable:wukong.pb.AuthRequest.cipher)
  return _internal_mutable_cipher();
}
inline const std::string& AuthRequest::_internal_cipher() const {
  return cipher_.Get();
}
inline void AuthRequest::_internal_set_cipher(const std::string& value) {
  
  cipher_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), value, GetArena());
}
inline void AuthRequest::set_cipher(std::string&& value) {
  
  cipher_.Set(
    &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::move(value), GetArena());
  // @@protoc_insertion_point(field_set_rvalue:wukong.pb.AuthRequest.cipher)
}
inline void AuthRequest::set_cipher(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  cipher_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArena());
  // @@protoc_insertion_point(field_set_char:wukong.pb.AuthRequest.cipher)
}
inline void AuthRequest::set_cipher(const char* value,
    size_t size) {
  
  cipher_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArena());
  // @@protoc_insertion_point(field_set_pointer:wukong.pb.AuthRequest.cipher)
}
inline std::string* AuthRequest::_internal_mutable_cipher() {
  
  return cipher_.Mutable(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline std::string* AuthRequest::release_cipher() {
  // @@protoc_insertion_point(field_release:wukong.pb.AuthRequest.cipher)
  return cipher_.Release(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void AuthRequest::set_allocated_cipher(std::string* cipher) {
  if (cipher != nullptr) {
    
  } else {
    
  }
  cipher_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), cipher,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:wukong.pb.AuthRequest.cipher)
}
inline std::string* AuthRequest::unsafe_arena_release_cipher() {
  // @@protoc_insertion_point(field_unsafe_arena_release:wukong.pb.AuthRequest.cipher)
  GOOGLE_DCHECK(GetArena() != nullptr);
  
  return cipher_.UnsafeArenaRelease(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArena());
}
inline void AuthRequest::unsafe_arena_set_allocated_cipher(
    std::string* cipher) {
  GOOGLE_DCHECK(GetArena() != nullptr);
  if (cipher != nullptr) {
    
  } else {
    
  }
  cipher_.UnsafeArenaSetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      cipher, GetArena());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:wukong.pb.AuthRequest.cipher)
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
