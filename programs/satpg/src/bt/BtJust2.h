#ifndef BTJUST2_H
#define BTJUST2_H

/// @file BtJust2.h
/// @brief BtJust2 のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2013 Yusuke Matsunaga
/// All rights reserved.


#include "BtJustBase.h"
#include "ym_utils/UnitAlloc.h"


BEGIN_NAMESPACE_YM_SATPG

//////////////////////////////////////////////////////////////////////
/// @class BtJust2 BtJust2.h "BtJust2.h"
/// @brief 必要なノードのみ正当化する BackTracer
//////////////////////////////////////////////////////////////////////
class BtJust2 :
  public BtJustBase
{
public:

  /// @brief コンストラクタ
  /// @param[in] tvmgr TvMgr
  /// @param[in] max_id ノードの最大 ID + 1 ( = TpgNetwork::node_num() )
  BtJust2(TvMgr& tvmgr,
	  ymuint max_id);

  /// @brief デストラクタ
  virtual
  ~BtJust2();


public:
  //////////////////////////////////////////////////////////////////////
  // BackTracer の仮想関数
  //////////////////////////////////////////////////////////////////////

  /// @brief バックトレースを行なう．
  /// @param[in] fnode 故障のあるノード
  /// @param[in] input_list テストパタンに関係のある入力のリスト
  /// @param[in] output_list 故障伝搬の可能性のある出力のリスト
  TestVector*
  operator()(TpgNode* fnode,
	     const vector<TpgNode*>& input_list,
	     const vector<TpgNode*>& output_list);


protected:
  //////////////////////////////////////////////////////////////////////
  // BtJustBase の仮想関数
  //////////////////////////////////////////////////////////////////////

  /// @brief clear_justified() 中で呼ばれるフック関数
  virtual
  void
  clear_justified_hook(TpgNode* node);


private:
  //////////////////////////////////////////////////////////////////////
  // 内部で用いるデータ構造
  //////////////////////////////////////////////////////////////////////

  struct NodeList
  {
    TpgNode* mNode;

    NodeList* mLink;
  };


private:
  //////////////////////////////////////////////////////////////////////
  // 内部で用いられる関数
  //////////////////////////////////////////////////////////////////////

  /// @brief solve 中で変数割り当ての正当化を行なう．
  /// @param[in] node 対象のノード
  /// @note node の値割り当てを正当化する．
  /// @note 正当化に用いられているノードには mark3 がつく．
  /// @note mark3 がついたノードは mBwdNodeList に格納される．
  NodeList*
  justify(TpgNode* node);

  /// @brief すべてのファンインに対して justify() を呼ぶ．
  /// @param[in] node 対象のノード
  NodeList*
  just_sub1(TpgNode* node);

  /// @brief 指定した値を持つのファンインに対して justify() を呼ぶ．
  /// @param[in] node 対象のノード
  /// @param[in] val 値
  NodeList*
  just_sub2(TpgNode* node,
	    Bool3 val);

  /// @brief justify の下請け関数
  /// @param[in] prim 対象のプリミティブ
  /// @param[in] node 対象のノード
  /// @note node の値割り当てを正当化する．
  /// @note 正当化に用いられているノードには mark3 がつく．
  /// @note mark3 がついたノードは mBwdNodeList に格納される．
  NodeList*
  justify_primitive(TpgPrimitive* prim,
		    TpgNode* node);

  /// @brief すべてのファンインに対して justify_primitive() を呼ぶ．
  /// @param[in] prim 対象のプリミティブ
  /// @param[in] node 対象のノード
  NodeList*
  jp_sub1(TpgPrimitive* prim,
	  TpgNode* node);

  /// @brief 指定した値を持つファンインに対して justify_primitive() を呼ぶ．
  /// @param[in] prim 対象のプリミティブ
  /// @param[in] node 対象のノード
  /// @param[in] val 値
  NodeList*
  jp_sub2(TpgPrimitive* prim,
	  TpgNode* node,
	  Bool3 val);

  /// @brief 新しいリストのセルを返す．
  NodeList*
  new_list_cell(TpgNode* node);

  /// @brief リストをマージする．
  void
  list_merge(NodeList*& dst_list,
	     NodeList* src_list);

  /// @brief リストのサイズを返す．
  static
  ymuint
  list_size(NodeList* node_list);

  /// @brief リストを削除する．
  void
  list_free(NodeList* node_list);


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // メモリ確保用のオブジェクト
  UnitAlloc mAlloc;

  // node->id() をキーにして入力番号のリストを納める配列
  vector<NodeList*> mJustArray;

};

END_NAMESPACE_YM_SATPG

#endif // BTJUST2_H
