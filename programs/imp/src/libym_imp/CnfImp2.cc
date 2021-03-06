﻿
/// @file CnfImp2.cc
/// @brief CnfImp2 の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "CnfImp2.h"
#include "YmLogic/CNFdd.h"
#include "YmLogic/CNFddMgr.h"
#include "YmNetworks/BdnMgr.h"
#include "YmNetworks/BdnNode.h"


BEGIN_NAMESPACE_YM_NETWORKS

//////////////////////////////////////////////////////////////////////
// クラス CnfImp2
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
CnfImp2::CnfImp2()
{
}

// @brief デストラクタ
CnfImp2::~CnfImp2()
{
}

// @brief ネットワーク中の間接含意を求める．
// @param[in] network 対象のネットワーク
// @param[in] imp_info 間接含意のリスト
void
CnfImp2::learning(const BdnMgr& network,
		  ImpInfo& imp_list)
{
  CNFddMgr mgr("cnfddmgr", "");

  // BDN から CNFdd を作る．
  ymuint n = network.max_node_id();
  for (ymuint i = 0; i < n; ++ i) {
    mgr.new_var(VarId(i));
  }

  vector<const BdnNode*> node_list;
  network.sort(node_list);

  CNFdd cnf = mgr.make_empty();
  for (vector<const BdnNode*>::iterator p = node_list.begin();
       p != node_list.end(); ++ p) {
    const BdnNode* bnode = *p;
    ymuint id = bnode->id();
    VarId v(id);
    Literal lit(v);

    const BdnNode* bnode0 = bnode->fanin0();
    bool inv0 = bnode->fanin0_inv();
    Literal lit0(VarId(bnode0->id()), inv0);

    const BdnNode* bnode1 = bnode->fanin1();
    bool inv1 = bnode->fanin1_inv();
    Literal lit1(VarId(bnode1->id()), inv1);

    if ( bnode->is_and() ) {
      CNFdd cnf0 = mgr.make_base();
      cnf0.add_literal(lit0);
      cnf0.add_literal(~lit);
      cnf |= cnf0;

      CNFdd cnf1 = mgr.make_base();
      cnf1.add_literal(lit1);
      cnf1.add_literal(~lit);
      cnf |= cnf1;

      CNFdd cnf2 = mgr.make_base();
      cnf2.add_literal(~lit0);
      cnf2.add_literal(~lit1);
      cnf2.add_literal(lit);
      cnf |= cnf2;
    }
    else if ( bnode->is_xor() ) {
      CNFdd cnf0 = mgr.make_base();
      cnf0.add_literal(~lit0);
      cnf0.add_literal(lit1);
      cnf0.add_literal(lit);
      cnf |= cnf0;

      CNFdd cnf1 = mgr.make_base();
      cnf1.add_literal(lit0);
      cnf1.add_literal(~lit1);
      cnf1.add_literal(lit);
      cnf |= cnf1;

      CNFdd cnf2 = mgr.make_base();
      cnf2.add_literal(~lit0);
      cnf2.add_literal(~lit1);
      cnf2.add_literal(~lit);
      cnf |= cnf2;

      CNFdd cnf3 = mgr.make_base();
      cnf3.add_literal(lit0);
      cnf3.add_literal(lit1);
      cnf3.add_literal(~lit);
      cnf |= cnf3;
    }
    else {
      ASSERT_NOT_REACHED;
    }
  }
#if 0
  cnf.print_clause(cout);
#endif
  cout << "Total " << cnf.count() << " clauses" << endl;

  for (ymuint i = 0; i < n; ++ i) {
    // i の肯定と否定のリテラルを含む節を求める．
    VarId v(i);
    Literal p_lit(v);
    Literal n_lit(~p_lit);
    CNFdd p_list = cnf.cofactor_p(v);
    CNFdd n_list = cnf.cofactor_n(v);
    cout << "Var#" << i << ": "
	 << p_list.count() << " x " << n_list.count() << endl;
#if 0
    cout << "p_list" << endl;
    p_list.print_clause(cout);
    cout << "n_list" << endl;
    n_list.print_clause(cout);
#endif
    CNFdd new_clause = p_list * n_list;
    new_clause.make_minimal();
    new_clause = new_clause.cut_off(3);
#if 0
    new_clause.print_clause(cout);
#endif
    cout << "  " << new_clause.count() << " learned implications"
	 << ", " << new_clause.node_count() << " nodes" << endl;
    cnf |= new_clause;
    cnf.make_minimal();
  }
  cout << "cnf" << endl;
  cnf.print_clause(cout);
  cout << endl;
}

END_NAMESPACE_YM_NETWORKS
