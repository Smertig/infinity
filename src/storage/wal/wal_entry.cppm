// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#include <typeinfo>

export module wal_entry;

import table_def;
import index_base;
import data_block;
import stl;

import infinity_exception;
import internal_types;

namespace infinity {

class BlockEntry;
class SegmentEntry;
enum class SegmentStatus;

export enum class WalCommandType : i8 {
    INVALID = 0,
    // -----------------------------
    // Catalog
    // -----------------------------
    CREATE_DATABASE = 1,
    DROP_DATABASE = 2,
    CREATE_TABLE = 3,
    DROP_TABLE = 4,
    ALTER_INFO = 5,
    CREATE_INDEX = 6,
    DROP_INDEX = 7,

    // -----------------------------
    // Data
    // -----------------------------
    IMPORT = 20,
    APPEND = 21,
    DELETE = 22,

    // -----------------------------
    // SEGMENT STATUS
    // -----------------------------
    SET_SEGMENT_STATUS_SEALED = 31,
    UPDATE_SEGMENT_BLOOM_FILTER_DATA = 32,

    // -----------------------------
    // Flush
    // -----------------------------
    CHECKPOINT = 99,
    COMPACT = 100,
};

export struct WalBlockInfo {
    BlockID block_id_{};
    u16 row_count_{};
    u16 row_capacity_{};
    TxnTimeStamp min_row_ts_{0};
    TxnTimeStamp max_row_ts_{0};
    TxnTimeStamp checkpoint_ts_{};
    u16 checkpoint_row_count_{0};
    Vector<i32> next_outline_idxes_;

    WalBlockInfo() = default;

    explicit WalBlockInfo(BlockEntry *block_entry);

    bool operator==(const WalBlockInfo &other) const;

    [[nodiscard]] i32 GetSizeInBytes() const;

    void WriteBufferAdv(char *&buf) const;

    static WalBlockInfo ReadBufferAdv(char *&ptr);

    String ToString() const;
};

export struct WalSegmentInfo {
    SegmentID segment_id_{};
    u64 column_count_{0};
    SizeT row_count_{0};
    SizeT actual_row_count_{0};
    SizeT row_capacity_{0};
    TxnTimeStamp min_row_ts_{0};
    TxnTimeStamp max_row_ts_{0};
    TxnTimeStamp commit_ts_{0};
    TxnTimeStamp deprecate_ts_{0};
    TxnTimeStamp begin_ts_{0};
    TransactionID txn_id_{0};
    Vector<WalBlockInfo> block_infos_;

    WalSegmentInfo() = default;

    explicit WalSegmentInfo(SegmentEntry *segment_entry);

    bool operator==(const WalSegmentInfo &other) const;

    [[nodiscard]] i32 GetSizeInBytes() const;

    void WriteBufferAdv(char *&buf) const;

    static WalSegmentInfo ReadBufferAdv(char *&ptr);

    String ToString() const;
};

// WalCommandType -> String
export struct WalCmd {
    virtual ~WalCmd() = default;

    virtual auto GetType() -> WalCommandType = 0;

    virtual auto operator==(const WalCmd &other) const -> bool { return typeid(*this) == typeid(other); }
    auto operator!=(const WalCmd &other) const -> bool { return !(*this == other); }
    // Estimated serialized size in bytes
    [[nodiscard]] virtual i32 GetSizeInBytes() const = 0;
    // Write to a char buffer
    virtual void WriteAdv(char *&ptr) const = 0;
    // Read from a serialized version
    static SharedPtr<WalCmd> ReadAdv(char *&ptr, i32 max_bytes);

    static String WalCommandTypeToString(WalCommandType type);
};

export struct WalCmdCreateDatabase : public WalCmd {
    explicit WalCmdCreateDatabase(String db_name, String db_dir_tail) : db_name_(std::move(db_name)), db_dir_tail_(std::move(db_dir_tail)) {}

    WalCommandType GetType() override { return WalCommandType::CREATE_DATABASE; }
    auto operator==(const WalCmd &other) const -> bool override {
        const auto *other_cmd = dynamic_cast<const WalCmdCreateDatabase *>(&other);
        return other_cmd != nullptr && IsEqual(db_name_, other_cmd->db_name_) && IsEqual(db_dir_tail_, other_cmd->db_dir_tail_);
    }
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String db_dir_tail_{};
};

export struct WalCmdDropDatabase : public WalCmd {
    explicit WalCmdDropDatabase(String db_name) : db_name_(std::move(db_name)) {}

    WalCommandType GetType() override { return WalCommandType::DROP_DATABASE; }
    bool operator==(const WalCmd &other) const override {
        auto other_cmd = dynamic_cast<const WalCmdDropDatabase *>(&other);
        return other_cmd != nullptr && IsEqual(db_name_, other_cmd->db_name_);
    }
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
};

export struct WalCmdCreateTable : public WalCmd {
    WalCmdCreateTable(String db_name, String table_dir_tail, const SharedPtr<TableDef> &table_def)
        : db_name_(std::move(db_name)), table_dir_tail_(std::move(table_dir_tail)), table_def_(table_def) {}

    WalCommandType GetType() override { return WalCommandType::CREATE_TABLE; }
    bool operator==(const WalCmd &other) const override;
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_dir_tail_{};
    SharedPtr<TableDef> table_def_{};
};

export struct WalCmdCreateIndex : public WalCmd {
    WalCmdCreateIndex(String db_name, String table_name, String index_dir_tail_, SharedPtr<IndexBase> index_base)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), index_dir_tail_(std::move(index_dir_tail_)),
          index_base_(std::move(index_base)) {}

    WalCommandType GetType() override { return WalCommandType::CREATE_INDEX; }

    bool operator==(const WalCmd &other) const override;

    [[nodiscard]] i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_name_{};
    String index_dir_tail_{};
    SharedPtr<IndexBase> index_base_{};
};

export struct WalCmdDropTable : public WalCmd {
    WalCmdDropTable(const String &db_name, const String &table_name) : db_name_(db_name), table_name_(table_name) {}

    WalCommandType GetType() override { return WalCommandType::DROP_TABLE; }
    bool operator==(const WalCmd &other) const override {
        auto other_cmd = dynamic_cast<const WalCmdDropTable *>(&other);
        return other_cmd != nullptr && IsEqual(db_name_, other_cmd->db_name_) && IsEqual(table_name_, other_cmd->table_name_);
    }
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_name_{};
};

export struct WalCmdDropIndex : public WalCmd {
    WalCmdDropIndex(const String &db_name, const String &table_name, const String &index_name)
        : db_name_(db_name), table_name_(table_name), index_name_(index_name) {}

    virtual WalCommandType GetType() override { return WalCommandType::DROP_INDEX; }

    bool operator==(const WalCmd &other) const override;

    i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    const String db_name_{};
    const String table_name_{};
    const String index_name_{};
};

export struct WalCmdImport : public WalCmd {
    WalCmdImport(String db_name, String table_name, WalSegmentInfo &&segment_info)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), segment_info_(std::move(segment_info)) {}

    WalCommandType GetType() override { return WalCommandType::IMPORT; }
    bool operator==(const WalCmd &other) const override;
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_name_{};
    WalSegmentInfo segment_info_;
};

export struct WalCmdAppend : public WalCmd {
    WalCmdAppend(String db_name, String table_name, const SharedPtr<DataBlock> &block)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), block_(block) {}

    WalCommandType GetType() override { return WalCommandType::APPEND; }
    bool operator==(const WalCmd &other) const override;
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_name_{};
    SharedPtr<DataBlock> block_{};
};

export struct WalCmdDelete : public WalCmd {
    WalCmdDelete(String db_name, String table_name, const Vector<RowID> &row_ids)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), row_ids_(row_ids) {}

    WalCommandType GetType() override { return WalCommandType::DELETE; }
    bool operator==(const WalCmd &other) const override;
    [[nodiscard]] i32 GetSizeInBytes() const override;
    void WriteAdv(char *&buf) const override;

    String db_name_{};
    String table_name_{};
    Vector<RowID> row_ids_{};
};

// used when append op turn an old unsealed segment full and sealed
// will always have necessary minmax filter
// may have user-defined bloom filter
export struct WalCmdSetSegmentStatusSealed : public WalCmd {
    WalCmdSetSegmentStatusSealed(String db_name,
                                 String table_name,
                                 SegmentID segment_id,
                                 String segment_filter_binary_data,
                                 Vector<Pair<BlockID, String>> block_filter_binary_data)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), segment_id_(segment_id),
          segment_filter_binary_data_(std::move(segment_filter_binary_data)), block_filter_binary_data_(std::move(block_filter_binary_data)) {}

    WalCommandType GetType() override { return WalCommandType::SET_SEGMENT_STATUS_SEALED; }

    bool operator==(const WalCmd &other) const override;

    i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    static WalCmdSetSegmentStatusSealed ReadBufferAdv(char *&ptr);

    const String db_name_{};
    const String table_name_{};
    const SegmentID segment_id_{};
    const String segment_filter_binary_data_{};
    const Vector<Pair<BlockID, String>> block_filter_binary_data_{};
};

// used when user-defined bloom filter need to be updated
export struct WalCmdUpdateSegmentBloomFilterData : public WalCmd {
    WalCmdUpdateSegmentBloomFilterData(String db_name,
                                       String table_name,
                                       SegmentID segment_id,
                                       String segment_filter_binary_data,
                                       Vector<Pair<BlockID, String>> block_filter_binary_data)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), segment_id_(segment_id),
          segment_filter_binary_data_(std::move(segment_filter_binary_data)), block_filter_binary_data_(std::move(block_filter_binary_data)) {}

    WalCommandType GetType() override { return WalCommandType::UPDATE_SEGMENT_BLOOM_FILTER_DATA; }

    bool operator==(const WalCmd &other) const override;

    i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    static WalCmdUpdateSegmentBloomFilterData ReadBufferAdv(char *&ptr);

    const String db_name_{};
    const String table_name_{};
    const SegmentID segment_id_{};
    const String segment_filter_binary_data_{};
    const Vector<Pair<BlockID, String>> block_filter_binary_data_{};
};

export struct WalCmdCheckpoint : public WalCmd {
    WalCmdCheckpoint(i64 max_commit_ts, bool is_full_checkpoint, String catalog_path)
        : max_commit_ts_(max_commit_ts), is_full_checkpoint_(is_full_checkpoint), catalog_path_(catalog_path) {}
    virtual WalCommandType GetType() override { return WalCommandType::CHECKPOINT; }

    virtual bool operator==(const WalCmd &other) const override;

    virtual i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    i64 max_commit_ts_{};
    bool is_full_checkpoint_;
    String catalog_path_{};
};

export struct WalCmdCompact : public WalCmd {
    WalCmdCompact(String &&db_name, String &&table_name, Vector<WalSegmentInfo> &&new_segment_infos, Vector<SegmentID> &&deprecated_segment_ids)
        : db_name_(std::move(db_name)), table_name_(std::move(table_name)), new_segment_infos_(std::move(new_segment_infos)),
          deprecated_segment_ids_(std::move(deprecated_segment_ids)) {}

    WalCommandType GetType() override { return WalCommandType::COMPACT; }

    bool operator==(const WalCmd &other) const override;

    i32 GetSizeInBytes() const override;

    void WriteAdv(char *&buf) const override;

    static WalCmdCompact ReadBufferAdv(char *&ptr);

    const String db_name_{};
    const String table_name_{};
    const Vector<WalSegmentInfo> new_segment_infos_{};
    const Vector<SegmentID> deprecated_segment_ids_{};
};

export struct WalEntryHeader {
    i32 size_{}; // size of payload, including the header, round to multi
    // of 4. There's 4 bytes pad just after the payload storing
    // the same value to assist backward iterating.
    u32 checksum_{}; // crc32 of the entry, including the header and the
    // payload. User shall populate it before writing to wal.
    i64 txn_id_{};    // txn id of the entry
    i64 commit_ts_{}; // commit timestamp of the txn
};

export struct WalEntry : WalEntryHeader {
    bool operator==(const WalEntry &other) const;

    bool operator!=(const WalEntry &other) const;

    // Estimated serialized size in bytes, ensured be no less than Write
    // requires, allowed be larger.
    [[nodiscard]] i32 GetSizeInBytes() const;

    // Write to a char buffer
    void WriteAdv(char *&ptr) const;
    // Read from a serialized version
    static SharedPtr<WalEntry> ReadAdv(char *&ptr, i32 max_bytes);

    Vector<SharedPtr<WalCmd>> cmds_{};

    [[nodiscard]] Pair<i64, String> GetCheckpointInfo() const;

    [[nodiscard]] bool IsCheckPoint() const;

    [[nodiscard]] bool IsFullCheckPoint() const;

    [[nodiscard]] String ToString() const;
};

export class WalEntryIterator {
public:
    static WalEntryIterator Make(const String &wal_path);

    [[nodiscard]] SharedPtr<WalEntry> Next();

private:
    WalEntryIterator(Vector<char> &&buf, std::streamsize wal_size) : buf_(std::move(buf)), wal_size_(wal_size) { end_ = buf_.data() + wal_size_; }

    Vector<char> buf_{};
    std::streamsize wal_size_{};
    char *end_{};
};

export class WalListIterator {
public:
    explicit WalListIterator(const Vector<String> &wal_list) {
        for (SizeT i = 0; i < wal_list.size(); ++i) {
            wal_deque_.push_back(wal_list[i]);
        }
    }

    [[nodiscard]] SharedPtr<WalEntry> Next();

private:
    Deque<String> wal_deque_{};
    UniquePtr<WalEntryIterator> iter_{};
};

} // namespace infinity
