/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef NDN_VSYNC_INTEREST_HELPER_HPP_
#define NDN_VSYNC_INTEREST_HELPER_HPP_

#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>

#include <ndn-cxx/name.hpp>

#include "vsync-common.hpp"

namespace ndn {
namespace vsync {

// Hepbers for view id

inline std::ostream& operator<<(std::ostream& os, const ViewID& vi) {
  return os << '(' << vi.first << ',' << vi.second << ')';
}

inline std::string ToString(const ViewID& vi) {
  std::ostringstream os;
  os << '(' << vi.first << ',' << vi.second << ')';
  return os.str();
}

// Helpers for version vector processing

inline VersionVector Merge(const VersionVector& v1, const VersionVector& v2) {
  if (v1.size() != v2.size()) return {};

  VersionVector res(v1.size());
  std::transform(v1.begin(), v1.end(), v2.begin(), res.begin(),
                 [](uint64_t l, uint64_t r) { return std::max(l, r); });
  return res;
}

inline std::string ToString(const VersionVector& v) {
  std::string s(1, '[');
  s.append(std::accumulate(std::next(v.begin()), v.end(), std::to_string(v[0]),
                           [](std::string a, uint64_t b) {
                             return a + ',' + std::to_string(b);
                           }))
      .append(1, ']');
  return s;
}

inline void EncodeVV(const VersionVector& v, proto::VV* vv_proto) {
  for (uint64_t n : v) {
    vv_proto->add_entry(n);
  }
}

inline void EncodeVV(const VersionVector& v, std::string& out) {
  proto::VV vv_proto;
  EncodeVV(v, &vv_proto);
  vv_proto.AppendToString(&out);
}

inline VersionVector DecodeVV(const proto::VV& vv_proto) {
  VersionVector vv(vv_proto.entry_size(), 0);
  for (int i = 0; i < vv_proto.entry_size(); ++i) {
    vv[i] = vv_proto.entry(i);
  }
  return vv;
}

inline VersionVector DecodeVV(const void* buf, size_t buf_size) {
  proto::VV vv_proto;
  if (!vv_proto.ParseFromArray(buf, buf_size)) return {};
  return DecodeVV(vv_proto);
}

struct VVCompare {
  bool operator()(const VersionVector& l, const VersionVector& r) const {
    if (l.size() != r.size()) return false;
    size_t equal_count = 0;
    for (size_t i = 0; i < l.size(); ++i) {
      if (l[i] > r[i])
        return false;
      else if (l[i] == r[i])
        ++equal_count;
    }
    if (equal_count == l.size())
      return false;
    else
      return true;
  }
};

// Helpers for interest processing

inline Name MakeSyncInterestName(const ViewID& vid, const std::string& digest) {
  // name = /[vsync_prefix]/digest/[view_num]/[leader_id]/[vector_clock_digest]
  Name n(kSyncPrefix);
  n.append("digest").appendNumber(vid.first).append(vid.second).append(digest);
  return n;
}

inline Name MakeVectorInterestName(const Name& sync_interest_name) {
  // name = /[vsync_prefix]/vector/[view_num]/[leader_id]/[vector_clock_digest]
  Name n(kSyncPrefix);
  n.append("vector")
      .append(sync_interest_name.get(-3))
      .append(sync_interest_name.get(-2))
      .append(sync_interest_name.get(-1));
  return n;
}

inline Name MakeViewInfoName(const ViewID& vid) {
  // name = /[vsync_prefix]/vinfo/[view_num]/[leader_id]/%00
  Name n(kSyncPrefix);
  n.append("vinfo").appendNumber(vid.first).append(vid.second).appendNumber(0);
  return n;
}

inline ViewID ExtractViewID(const Name& n) {
  uint64_t view_num = n.get(-3).toNumber();
  std::string leader_id = n.get(-2).toUri();
  return {view_num, leader_id};
}

inline std::string ExtractVectorDigest(const Name& n) {
  return n.get(-1).toUri();
}

// Helpers for data processing

inline Name MakeDataName(const Name& prefix, const NodeID& nid,
                         const ViewID& vid, uint64_t seq) {
  // name = /node_prefix/node_id/view_num/leader_id/seq_num
  Name n(prefix);
  n.append(nid).appendNumber(vid.first).append(vid.second).appendNumber(seq);
  return n;
}

inline Name ExtractNodePrefix(const Name& n) { return n.getPrefix(-4); }

inline NodeID ExtractNodeID(const Name& n) { return n.get(-4).toUri(); }

inline uint64_t ExtractSequenceNumber(const Name& n) {
  return n.get(-1).toNumber();
}

inline void EncodeESN(const ESN& ldi, std::string& out) {
  proto::ESN esn_proto;
  esn_proto.set_view_num(ldi.vi.first);
  esn_proto.set_leader_id(ldi.vi.second);
  esn_proto.set_seq_num(ldi.seq);
  esn_proto.AppendToString(&out);
}

inline std::pair<ESN, bool> DecodeESN(const void* buf, size_t buf_size) {
  proto::ESN esn_proto;
  if (!esn_proto.ParseFromArray(buf, buf_size)) return {};

  ESN esn;
  esn.vi.first = esn_proto.view_num();
  esn.vi.second = esn_proto.leader_id();
  esn.seq = esn_proto.seq_num();
  return {esn, true};
}

inline std::string ToString(const ESN& s) {
  std::ostringstream os;
  os << '{' << s.vi << ',' << s.seq << '}';
  return os.str();
}

inline std::ostream& operator<<(std::ostream& os, const ESN& s) {
  return os << '{' << s.vi << ',' << s.seq << '}';
}

inline bool operator==(const ESN& l, const ESN& r) {
  return l.vi == r.vi && l.seq == r.seq;
}

}  // namespace vsync
}  // namespace ndn

#endif  // NDN_VSYNC_INTEREST_HELPER_HPP_
