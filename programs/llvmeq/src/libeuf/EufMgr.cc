﻿
/// @file EufMgr.cc
/// @brief EufMgr の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2013 Yusuke Matsunaga
/// All rights reserved.


#include "EufMgr.h"
#include "EufBinMgr.h"
#include "EufFuncMgr.h"
#include "EufVarMgr.h"
#include "EufNode.h"
#include "YmUtils/MFSet.h"


BEGIN_NAMESPACE_YM_LLVMEQ

//////////////////////////////////////////////////////////////////////
// クラス EufMgr
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
// @param[in] sat_type SAT-solver の種類を表す文字列
// @param[in] sat_opt SAT-solver に渡すオプション文字列
// @param[in] sat_log ログの出力用ストリーム
EufMgr::EufMgr(const string& sat_type,
	       const string& sat_opt,
	       ostream* sat_log) :
  mSolver(sat_type, sat_opt, sat_log)
{
  mVarMgr = new EufVarMgr;
  mFuncMgr = new EufFuncMgr;
  mBinMgr = new EufBinMgr;
  mLastId = 0;
}

// @brief デストラクタ
EufMgr::~EufMgr()
{
  delete mVarMgr;
  delete mFuncMgr;
  delete mBinMgr;
}

// @brief 変数を生成する．
// @param[in] name 変数名
// @param[in] bool_flag ブール変数の時 true にするフラグ
EufNode*
EufMgr::new_variable(const string& name,
		     bool bool_flag)
{
  EufNode* var = mVarMgr->find(name);
  if ( var == NULL ) {
    VarId vid;
    if ( bool_flag ) {
      vid = mSolver.new_var();
    }
    var = mVarMgr->new_variable(mLastId, vid, name, bool_flag);
    ++ mLastId;
  }
  return var;
}

BEGIN_NONAMESPACE

// EufNode を整列させるためのファンクタ
struct Lt
{
  bool
  operator()(EufNode* left,
	     EufNode* right)
  {
    return left->id() < right->id();
  }
};

END_NONAMESPACE

// @brief 関数を生成する．
// @param[in] name 関数名
// @param[in] arg_list 引数のリスト
// @param[in] sflag 対称関数の時 true にするフラグ
// @note sflag が true の時は arg_list の順番を正規化する．
EufNode*
EufMgr::new_function(const string& name,
		     const vector<EufNode*>& arg_list,
		     bool sflag)
{
  // ソートするかもしれないのでローカルな変数にコピーする．
  vector<EufNode*> tmp_list(arg_list.begin(), arg_list.end());

  if ( sflag ) {
    // ID番号でソートする．
    sort(tmp_list.begin(), tmp_list.end(), Lt());
  }

  EufNode* node = mFuncMgr->find(name, tmp_list);
  if ( node == NULL ) {
    node = mFuncMgr->new_function(mLastId, name, tmp_list);
    ++ mLastId;

    mFuncList.push_back(node);
  }

  return node;
}

// @brief 等価演算子を生成する．
// @param[in] left 左辺式
// @param[in] right 右辺式
// @note 場合によっては左辺式と右辺式は交換される．
EufNode*
EufMgr::new_equality(EufNode* left,
		     EufNode* right)
{
  ASSERT_COND( left->is_boolean() == right->is_boolean() );

  // left と right は対称なので正規化する．
  if ( left->id() > right->id() ) {
    EufNode* tmp_node = left;
    left = right;
    right = tmp_node;
  }

  EufNode* node = mBinMgr->find_equality(left, right);
  if ( node == NULL ) {
    VarId vid = mSolver.new_var();
    node = mBinMgr->new_equality(mLastId, vid, left, right);
    ++ mLastId;

    mEqList.push_back(node);
  }
  return node;
}

// @brief 連言(conjunction)を生成する．
// @param[in] left 左辺式
// @param[in] right 右辺式
EufNode*
EufMgr::new_conjunction(EufNode* left,
			EufNode* right)
{
  ASSERT_COND( left->is_boolean() );
  ASSERT_COND( right->is_boolean() );

  // left と right は対称なので正規化する．
  if ( left->id() > right->id() ) {
    EufNode* tmp_node = left;
    left = right;
    right = tmp_node;
  }

  EufNode* node = mBinMgr->find_conjunction(left, right);
  if ( node == NULL ) {
    VarId vid = mSolver.new_var();
    node = mBinMgr->new_conjunction(mLastId, vid, left, right);
    ++ mLastId;

    Literal olit(vid);
    Literal llit(left->var_id());
    Literal rlit(right->var_id());
    mSolver.add_clause(~llit, ~rlit, olit);
    mSolver.add_clause( llit, ~olit);
    mSolver.add_clause( rlit, ~olit);
  }
  return node;
}

// @brief 選言(disjunction)を生成する．
// @param[in] left 左辺式
// @param[in] right 右辺式
EufNode*
EufMgr::new_disjunction(EufNode* left,
			EufNode* right)
{
  ASSERT_COND( left->is_boolean() );
  ASSERT_COND( right->is_boolean() );

  // left と right は対称なので正規化する．
  if ( left->id() > right->id() ) {
    EufNode* tmp_node = left;
    left = right;
    right = tmp_node;
  }

  EufNode* node = mBinMgr->find_disjunction(left, right);
  if ( node == NULL ) {
    VarId vid = mSolver.new_var();
    node = mBinMgr->new_disjunction(mLastId, vid, left, right);
    ++ mLastId;

    Literal olit(vid);
    Literal llit(left->var_id());
    Literal rlit(right->var_id());
    mSolver.add_clause( llit,  rlit, ~olit);
    mSolver.add_clause(~llit,  olit);
    mSolver.add_clause(~rlit,  olit);
  }
  return node;
}

// @brief 否定(negation)を生成する．
// @param[in] operand 式
EufNode*
EufMgr::new_negation(EufNode* operand)
{
  ASSERT_COND( operand->is_boolean() );

  EufNode* node = mBinMgr->find_negation(operand);
  if ( node == NULL ) {
    VarId vid = mSolver.new_var();
    node = mBinMgr->new_negation(mLastId, vid, operand);
    ++ mLastId;

    Literal olit(vid);
    Literal llit(operand->var_id());
    mSolver.add_clause( llit,  olit);
    mSolver.add_clause(~llit, ~olit);
  }
  return node;
}

// @brief validity check を行う．
// @param[in] assumption 仮定
// @param[in] node 対象のノード
// @note node->is_boolean() が true である必要がある．
bool
EufMgr::check_validity(const vector<EufNode*>& node_assumption,
		       EufNode* node)
{
  ASSERT_COND( node->is_boolean() );

  vector<Literal> assumption;
  assumption.reserve(node_assumption.size() + 1);
  for (vector<EufNode*>::const_iterator p = node_assumption.begin();
       p != node_assumption.end(); ++ p) {
    EufNode* node = *p;
    assumption.push_back(Literal(node->var_id()));
  }
  Literal vlit(node->var_id());
  assumption.push_back(~vlit);

  vector<Bool3> model;
  for ( ; ; ) {
    Bool3 sat_stat = mSolver.solve(assumption, model);
    if ( sat_stat == kB3False ) {
      // vlit の否定が unsat ということは vlit は valid
      cout << "Valid" << endl;
      return true;
    }
    if ( sat_stat == kB3X ) {
      // アボート
      cout << "Aborted" << endl;
      return false;
    }

    // model の内容から成り立っている等価式を求める．
    vector<EufNode*> eq_list;
    vector<EufNode*> neq_list;
    for (vector<EufNode*>::iterator p = mEqList.begin();
	 p != mEqList.end(); ++ p) {
      EufNode* eq_node = *p;
      Bool3 val = model[eq_node->var_id().val()];
      if ( val == kB3True ) {
	eq_list.push_back(eq_node);
	cout << "#" << eq_node->id() << " is true" << endl;
      }
      else if ( val == kB3False ) {
	neq_list.push_back(eq_node);
	cout << "#" << eq_node->id() << " is false" << endl;
      }
    }

    // eq_list 内の等価式から congruence closure を求める．
    MFSet mfset(mLastId);
    for (vector<EufNode*>::iterator p = eq_list.begin();
	 p != eq_list.end(); ++ p) {
      EufNode* eq_node = *p;
      mfset.merge(eq_node->left()->id(), eq_node->right()->id());
    }

    for ( ; ; ) {
      bool change = false;
      ymuint nf = mFuncList.size();
      for (ymuint i = 0; i < nf; ++ i) {
	EufNode* f1 = mFuncList[i];
	ymuint n = f1->arg_num();
	for (ymuint j = i + 1; j < nf; ++ j) {
	  EufNode* f2 = mFuncList[j];
	  if ( f2->arg_num() != n ) {
	    continue;
	  }
	  if ( mfset.find(f1->id()) == mfset.find(f2->id()) ) {
	    continue;
	  }
	  bool eq = true;
	  for (ymuint k = 0; k < n; ++ k) {
	    EufNode* a1 = f1->arg(k);
	    EufNode* a2 = f2->arg(k);
	    ymuint r1 = mfset.find(a1->id());
	    ymuint r2 = mfset.find(a2->id());
	    if ( r1 != r2 ) {
	      eq = false;
	      break;
	    }
	  }
	  if ( eq ) {
	    mfset.merge(f1->id(), f2->id());
	    change = true;
	  }
	}
      }
      if ( !change ) {
	break;
      }
    }

    // 非等価なノードペアが同値類に含まれていたら矛盾
    bool conflict = false;
    for (vector<EufNode*>::iterator p = neq_list.begin();
	 p != neq_list.end(); ++ p) {
      EufNode* neq_node = *p;
      ymuint r1 = mfset.find(neq_node->left()->id());
      ymuint r2 = mfset.find(neq_node->right()->id());
      cout << "#" << neq_node->id()
	   << "->left() = "
	   << r1 << endl
	   << "  ->right() = "
	   << r2 << endl;
      if ( r1 == r2 ) {
	cout << " conflict" << endl;
	vector<Literal> tmp_list;
	tmp_list.reserve(eq_list.size() + 1);
	cout << " add ";
	const char* plus = "";
	for (vector<EufNode*>::iterator q = eq_list.begin();
	     q != eq_list.end(); ++ q) {
	  EufNode* eq_node = *q;
	  Literal eq_lit(eq_node->var_id());
	  tmp_list.push_back(~eq_lit);
	  cout << plus << ~eq_lit;
	  plus = " + ";
	}
	Literal neq_lit(neq_node->var_id());
	tmp_list.push_back(neq_lit);
	cout << " + " << neq_lit << endl;
	mSolver.add_clause(tmp_list);
	conflict = true;
      }
    }
    if ( !conflict ) {
      // 命題論理部分も等価式も矛盾がない．
      cout << "Invalid" << endl;
      return false;
    }
  }
}


END_NAMESPACE_YM_LLVMEQ
