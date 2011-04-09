
/// @file libym_mvn/MvnConstBitSelect.cc
/// @brief MvnConstBitSelect の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010 Yusuke Matsunaga
/// All rights reserved.


#include "MvnConstBitSelect.h"
#include "ym_mvn/MvnMgr.h"


BEGIN_NAMESPACE_YM_MVN

// @brief コンストラクタ
// @param[in] module 親のモジュール
// @param[in] bitpos ビット位置
MvnConstBitSelect::MvnConstBitSelect(MvnModule* module,
				     ymuint bitpos) :
  MvnNodeBase(module, MvnNode::kConstBitSelect, 1, 1),
  mBitPos(bitpos)
{
}

// @brief デストラクタ
MvnConstBitSelect::~MvnConstBitSelect()
{
}

// @brief ビット位置を得る．
// @note type() が kConstBitSelect の時のみ意味を持つ．
// @note デフォルトの実装では 0 を返す．
ymuint
MvnConstBitSelect::bitpos() const
{
  return mBitPos;
}

// @brief bit-selectノードを生成する．
// @param[in] module ノードが属するモジュール
// @param[in] bit_width ビット幅
// @param[in] bitpos ビット位置
// @return 生成したノードを返す．
MvnNode*
MvnMgr::new_constbitselect(MvnModule* module,
			   ymuint bit_width,
			   ymuint bitpos)
{
  MvnNode* node = new MvnConstBitSelect(module, bitpos);
  reg_node(node);

  node->_input(0)->mBitWidth = bit_width;
  node->_output(0)->mBitWidth = 1;

  return node;
}

END_NAMESPACE_YM_MVN
