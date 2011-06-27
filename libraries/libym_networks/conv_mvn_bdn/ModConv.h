#ifndef LIBYM_NETWORKSBDNCONV_MODCONV_H
#define LIBYM_NETWORKSBDNCONV_MODCONV_H

/// @file libym_networks/ModConv.h
/// @brief ModConv のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "MvnConv.h"


BEGIN_NAMESPACE_YM_NETWORKSBDNCONV

//////////////////////////////////////////////////////////////////////
/// @class ModConv ModConv.h "ModConv.h"
/// @brief 剰余算ノードに対する処理を行う MvnConv の派生クラス
//////////////////////////////////////////////////////////////////////
class ModConv :
  public MvnConv
{
public:

  /// @brief コンストラクタ
  ModConv();

  /// @brief デストラクタ
  virtual
  ~ModConv();


public:
  //////////////////////////////////////////////////////////////////////
  // MvnConv の仮想関数
  //////////////////////////////////////////////////////////////////////

  /// @brief MvnNode を BdnMgr に変換する．
  /// @param[in] node ノード
  /// @param[in] bdnetwork 変換結果の BdnMgr
  /// @param[in] nodemap ノードの対応関係を表すマップ
  /// @retval true このクラスで変換処理を行った．
  /// @retval false このクラスでは変換処理を行わなかった．
  virtual
  bool
  operator()(const MvnNode* node,
	     BdnMgr& bdnetwork,
	     MvnBdnMap& nodemap);

};


END_NAMESPACE_YM_NETWORKSBDNCONV

#endif // LIBYM_NETWORKSBDNCONV_MODCONV_H