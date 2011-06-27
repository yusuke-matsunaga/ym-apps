#ifndef YM_NETWORKSBDNCONV_MVNBDNCONV_NSDEF_H
#define YM_NETWORKSBDNCONV_MVNBDNCONV_NSDEF_H

/// @file ym_networks/mvnbdnconv_nsdef.h
/// @brief ym_mvnbdnconv の共通ヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// $Id: bdn.h 2507 2009-10-17 16:24:02Z matsunaga $
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "ymtools.h"


/// @brief ym_mvnbdnconv 用の名前空間の開始
#define BEGIN_NAMESPACE_YM_NETWORKSBDNCONV \
BEGIN_NAMESPACE_YM \
BEGIN_NAMESPACE(nsMvnBdnConv)

/// @brief ym_mvnbdnconv 用の名前空間の終了
#define END_NAMESPACE_YM_NETWORKSBDNCONV \
END_NAMESPACE(nsMvnBdnConv) \
END_NAMESPACE_YM

/// @namespace nsYm::nsMvnBdnConv
/// @brief MvnBdnConv 関係のクラスが属する名前空間
///
/// ただし，主要なものは nsYm にインポートされている．
/// 実装用のローカルな要素だけがこの名前空間に存在する．

BEGIN_NAMESPACE_YM_NETWORKSBDNCONV

class MvnBdnConv;
class MvnBdnMap;

END_NAMESPACE_YM_NETWORKSBDNCONV

BEGIN_NAMESPACE_YM

using nsMvnBdnConv::MvnBdnConv;
using nsMvnBdnConv::MvnBdnMap;

END_NAMESPACE_YM

#endif // YM_NETWORKSBDNCONV_MVNBDNCONV_NSDEF_H