#ifndef LIBYM_NETWORKS_CMN_CMNNODEOUTPUT_H
#define LIBYM_NETWORKS_CMN_CMNNODEOUTPUT_H

/// @file libym_networks/cmn/CmnNodeOutput.h
/// @brief CmnNodeOutput のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "ym_networks/CmnNode.h"
#include "ym_networks/CmnEdge.h"


BEGIN_NAMESPACE_YM_NETWORKS

//////////////////////////////////////////////////////////////////////
/// @class CmnNodeOutput CmnNodeOutput.h "CmnNodeOutput.h"
/// @brief 出力を表す CmnNode の派生クラス
//////////////////////////////////////////////////////////////////////
class CmnNodeOutput :
  public CmnNode
{
  friend class CmnMgrImpl;

protected:

  /// @brief コンストラクタ
  CmnNodeOutput();

  /// @brief デストラクタ
  virtual
  ~CmnNodeOutput();


public:

  /// @brief タイプを得る．
  virtual
  tType
  type() const;

  /// @brief 出力ノードの時に true を返す．
  virtual
  bool
  is_output() const;

  /// @brief ファンイン数を得る．
  virtual
  ymuint
  ni() const;

  /// @brief ファンインのノードを得る．
  /// @param[in] pos 入力番号
  /// @return pos 番めのファンインのノード
  /// @note 該当するファンインがなければ NULL を返す．
  virtual
  const CmnNode*
  fanin(ymuint pos) const;

  /// @brief ファンインの枝を得る．
  /// @param[in] pos 入力番号
  /// @return pos 番目の入力の枝
  /// @note 該当するファンインの枝がなければ NULL を返す．
  virtual
  const CmnEdge*
  fanin_edge(ymuint pos) const;

  /// @brief ファンインの枝を得る．
  /// @param[in] pos 入力番号
  /// @return pos 番目の入力の枝
  /// @note 該当するファンインの枝がなければ NULL を返す．
  virtual
  CmnEdge*
  _fanin_edge(ymuint pos);


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // ファンインの枝
  CmnEdge mFanin;

};

END_NAMESPACE_YM_NETWORKS


#endif // LIBYM_NETWORKS_CMN_CMNNODEOUTPUT_H
