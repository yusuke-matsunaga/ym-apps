﻿
/// @file LsimBdd2.cc
/// @brief LsimBdd2 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "LsimBdd2.h"
#include "YmNetworks/BdnNode.h"
#include "YmNetworks/BdnPort.h"


BEGIN_NAMESPACE_YM

//////////////////////////////////////////////////////////////////////
// クラス LsimBdd2
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
LsimBdd2::LsimBdd2() :
  mBddMgr("bmc", "Bdd Manager")
{
}

// @brief デストラクタ
LsimBdd2::~LsimBdd2()
{
}


BEGIN_NONAMESPACE

inline
ympuint
encode(LsimBdd2::Bdd2Node* node,
       bool inv)
{
  return reinterpret_cast<ympuint>(node) | inv;
}

inline
LsimBdd2::Bdd2Node*
decode_node(ympuint val)
{
  return reinterpret_cast<LsimBdd2::Bdd2Node*>(val & ~1UL);
}

inline
bool
decode_inv(ympuint val)
{
  return static_cast<bool>(val & 1UL);
}

END_NONAMESPACE

// @brief ネットワークをセットする．
// @param[in] bdn 対象のネットワーク
// @param[in] order_map 順序マップ
void
LsimBdd2::set_network(const BdnMgr& bdn,
		      const unordered_map<string, ymuint>& order_map)
{
  ymuint n = bdn.max_node_id();
  vector<Bdd> bddmap(n);

  const BdnNodeList& input_list = bdn.input_list();

  if ( order_map.empty() ) {
    ymuint id = 0;
    for (BdnNodeList::const_iterator p = input_list.begin();
	 p != input_list.end(); ++ p) {
      const BdnNode* node = *p;
      Bdd bdd = mBddMgr.make_posiliteral(VarId(id));
      ++ id;
      bddmap[node->id()] = bdd;
    }
  }
  else {
    for (BdnNodeList::const_iterator p = input_list.begin();
	 p != input_list.end(); ++ p) {
      const BdnNode* node = *p;
      string name = node->port()->name();
      unordered_map<string, ymuint>::const_iterator q = order_map.find(name);
      if ( q == order_map.end() ) {
	cerr << "No order for " << name << endl;
	abort();
      }
      ymuint id = q->second;
      Bdd bdd = mBddMgr.make_posiliteral(VarId(id));
      bddmap[node->id()] = bdd;
    }
  }

  vector<const BdnNode*> node_list;
  bdn.sort(node_list);
  for (vector<const BdnNode*>::const_iterator p = node_list.begin();
       p != node_list.end(); ++ p) {
    const BdnNode* node = *p;
    const BdnNode* fanin0 = node->fanin0();
    Bdd bdd0 = bddmap[fanin0->id()];
    if ( node->fanin0_inv() ) {
      bdd0 = ~bdd0;
    }
    const BdnNode* fanin1 = node->fanin1();
    Bdd bdd1 = bddmap[fanin1->id()];
    if ( node->fanin1_inv() ) {
      bdd1 = ~bdd1;
    }
#if 0
    cout << " " << bdd0.size() << " x " << bdd1.size();
    cout.flush();
#endif
    if ( node->is_and() ) {
      Bdd bdd = bdd0 & bdd1;
      bddmap[node->id()] = bdd;
    }
    else if ( node->is_xor() ) {
      Bdd bdd = bdd0 ^ bdd1;
      bddmap[node->id()] = bdd;
    }
    else {
      ASSERT_NOT_REACHED;
    }
#if 0
    cout << " = " << bddmap[node->id()].size();
#endif
  }

  mBddMgr.disable_gc();

  unordered_map<Bdd, ympuint> node_map;

  const BdnNodeList& output_list = bdn.output_list();
  ymuint no = output_list.size();
  mOutputList.clear();
  mOutputList.reserve(no);
  mNodeList.clear();
  for (BdnNodeList::const_iterator p = output_list.begin();
       p != output_list.end(); ++ p) {
    BdnNode* node = *p;
    BdnNode* node0 = node->output_fanin();
    Bdd bdd;
    if ( node0 != NULL ) {
      bdd = bddmap[node0->id()];
    }
    else {
      bdd = mBddMgr.make_zero();
    }
    if ( node->output_fanin_inv() ) {
      bdd = ~bdd;
    }
    ympuint ptr = make_node(bdd, node_map);
    mOutputList.push_back(ptr);
  }

  cout << "Node size: " << mNodeList.size() << endl;
}

ympuint
LsimBdd2::make_node(Bdd bdd,
		    unordered_map<Bdd, ympuint>& node_map)
{
  if ( bdd.is_zero() ) {
    return 0UL;
  }
  if ( bdd.is_one() ) {
    return 1UL;
  }

  unordered_map<Bdd, ympuint>::iterator p = node_map.find(bdd);
  if ( p != node_map.end() ) {
    return p->second;
  }

  p = node_map.find(~bdd);
  if ( p != node_map.end() ) {
    return p->second ^ 1UL;
  }

  Bdd bdd0;
  Bdd bdd1;
  VarId varid0 = bdd.root_decomp(bdd0, bdd1);

  VarId varid2(0);
  Bdd bdd00;
  Bdd bdd01;
  ympuint node00;
  ympuint node01;
  if ( bdd0.is_zero() ) {
    node00 = node01 = 0UL;
  }
  else if ( bdd0.is_one() ) {
    node00 = node01 = 1UL;
  }
  else {
    varid2 = bdd0.root_decomp(bdd00, bdd01);
  }

  VarId varid3(0);
  Bdd bdd10;
  Bdd bdd11;
  ympuint node10;
  ympuint node11;
  if ( bdd1.is_zero() ) {
    node10 = node11 = 0UL;
  }
  else if ( bdd1.is_one() ) {
    node10 = node11 = 1UL;
  }
  else {
    varid3 = bdd1.root_decomp(bdd10, bdd11);
  }

  VarId varid1 = varid2;
  if ( varid1 == VarId(0) ) {
    varid1 = varid3;
  }
  if ( varid1 > varid3 ) {
    varid1 = varid3;
  }

  if ( varid2 == VarId(0) || varid2 != varid1 ) {
    node00 = node01 = make_node(bdd0, node_map);
  }
  else {
    node00 = make_node(bdd00, node_map);
    node01 = make_node(bdd01, node_map);
  }
  if ( varid3 == VarId(0) || varid3 != varid1 ) {
    node10 = node11 = make_node(bdd1, node_map);
  }
  else {
    node10 = make_node(bdd10, node_map);
    node11 = make_node(bdd11, node_map);
  }

#if 0
  mNodeList.push_back(Bdd2Node(varid0, varid1, node00, node01, node10, node11));
  ympuint ptr = encode(&mNodeList.back(), false);
#else
  Bdd2Node* node = new Bdd2Node(varid0, varid1, node00, node01, node10, node11);
  mNodeList.push_back(node);
  ympuint ptr = encode(node, false);
#endif

  node_map.insert(make_pair(bdd, ptr));

  return ptr;
}

BEGIN_NONAMESPACE

ymuint total_step;
ymuint total_num;

ymuint64
eval_bdd(ympuint ptr0,
	 const vector<ymuint64>& iv)
{
  ymuint val = 0U;
  for (ymuint b = 0; b < 64; ++ b) {
    ympuint ptr = ptr0;
    ymuint64 bit = 1UL << b;
    ++ total_num;
    for ( ; ; ) {
      ++ total_step;
      if ( ptr == 0UL ) {
	break;
      }
      else if ( ptr == 1UL ) {
	val |= bit;
	break;
      }
      LsimBdd2::Bdd2Node* node = decode_node(ptr);
      bool inv = decode_inv(ptr);
      VarId id0 = node->mId[0];
      VarId id1 = node->mId[1];
      ymuint64 ival0 = iv[id0.val()];
      ymuint64 ival1 = iv[id1.val()];
      ymuint addr = 0U;
      if ( ival0 & bit ) {
	addr |= 2U;
      }
      if ( ival1 & bit ) {
	addr |= 1U;
      }
      ptr = node->mFanins[addr];
      if ( inv ) {
	ptr = ptr ^ 1UL;
      }
    }
  }
  return val;
}

END_NONAMESPACE

// @brief 論理シミュレーションを行う．
// @param[in] iv 入力ベクタ
// @param[out] ov 出力ベクタ
void
LsimBdd2::eval(const vector<ymuint64>& iv,
	       vector<ymuint64>& ov)
{
  total_step = 0;
  total_num = 0;
  ymuint no = ov.size();
  for (ymuint i = 0; i < no; ++ i) {
    ympuint ptr = mOutputList[i];
    ov[i] = eval_bdd(ptr, iv);
  }
#if 0
  cout << "total_step = " << total_step << endl
       << "total_num  = " << total_num << endl
       << "ave. step  = " << static_cast<double>(total_step) / total_num << endl;
#endif
}

END_NAMESPACE_YM
