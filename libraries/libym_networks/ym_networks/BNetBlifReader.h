#ifndef YM_NETWORKS_BNETBLIFREADER_H
#define YM_NETWORKS_BNETBLIFREADER_H

/// @file ym_networks/BNetBlifReader.h
/// @brief BNetBlifReader のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "ym_networks/bnet.h"


BEGIN_NAMESPACE_YM_NETWORKS

namespace nsBlif {
  class BlifParser;
  class BNetBlifHandler;
}


//////////////////////////////////////////////////////////////////////
/// @class BNetBlifReader BNetBlifReader.h "ym_networks/BNetBlifReader.h"
/// @ingroup BnetGroup
/// @brief blif 形式のファイルを読み込んで BNetwork に設定するクラス
/// @sa BNetwork
//////////////////////////////////////////////////////////////////////
class BNetBlifReader
{
public:

  /// @brief コンストラクタ
  BNetBlifReader();

  /// @brief デストラクタ
  ~BNetBlifReader();


public:

  /// @brief blif 形式のファイルを読み込む
  /// @param[in] filename ファイル名
  /// @param[in] network 読み込んだ内容を設定するネットワーク
  /// @retval true 正常に読み込めた
  /// @retval false 読み込み中にエラーが起こった．
  bool
  read(const string& filename,
       BNetwork& network);


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // blif パーサー
  nsBlif::BlifParser* mParser;

  // ハンドラ
  nsBlif::BNetBlifHandler* mHandler;

};

END_NAMESPACE_YM_NETWORKS

#endif // YM_NETWORKS_BNETBLIFREADER_H
