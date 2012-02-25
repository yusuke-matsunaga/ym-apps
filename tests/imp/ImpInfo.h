#ifndef IMPINFO_H
#define IMPINFO_H

/// @file ImpInfo.h
/// @brief ImpInfo のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2012 Yusuke Matsunaga
/// All rights reserved.


#include "ym_networks/bdn.h"
#include "ImpVal.h"
#include "ym_utils/UnitAlloc.h"


BEGIN_NAMESPACE_YM_NETWORKS

class ImpCell;
class ImpList;

//////////////////////////////////////////////////////////////////////
/// @class ImpInfo ImpInfo.h "ImpInfo.h"
/// @brief 含意情報を表すクラス
//////////////////////////////////////////////////////////////////////
class ImpInfo
{
public:

  /// @brief 空のコンストラクタ
  ImpInfo();

  /// @brief デストラクタ
  ~ImpInfo();


public:
  //////////////////////////////////////////////////////////////////////
  // 値を取り出す関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 含意情報のリストを取り出す．
  /// @param[in] src_id 含意元のノード番号
  /// @param[in] src_val 含意元の値 ( 0 or 1 )
  const ImpList&
  get(ymuint src_id,
      ymuint src_val) const;

  /// @brief 該当する含意情報が含まれているか調べる．
  bool
  check(ymuint src_id,
	ymuint src_val,
	ymuint dst_id,
	ymuint dst_val) const;

  /// @brief 含意の総数を得る．
  ymuint
  size() const;

  /// @brief 内容を出力する．
  /// @param[in] s 出力先のストリーム
  void
  print(ostream& s) const;


public:
  //////////////////////////////////////////////////////////////////////
  // 値を設定する関数
  //////////////////////////////////////////////////////////////////////

  /// @brief 内容をクリアする．
  void
  clear();

  /// @brief サイズを設定する．
  /// @param[in] max_id ID番号の最大値
  void
  set_size(ymuint max_id);

  /// @brief 含意情報を追加する．
  /// @param[in] src_id 含意元のノード番号
  /// @param[in] src_val 含意元の値 ( 0 or 1 )
  /// @param[in] dst_id 含意先のノード番号
  /// @param[in] dst_val 含意先の値 ( 0 or 1 )
  void
  put(ymuint src_id,
      ymuint src_val,
      ymuint dst_id,
      ymuint dst_val);

  /// @brief 含意情報を追加する．
  /// @param[in] src_id 含意元のノード番号
  /// @param[in] src_val 含意元の値 ( 0 or 1 )
  /// @param[in] imp_list 含意リスト
  void
  put(ymuint src_id,
      ymuint src_val,
      const vector<ImpVal>& imp_list);


private:
  //////////////////////////////////////////////////////////////////////
  // 内部で用いられる関数
  //////////////////////////////////////////////////////////////////////

  /// @brief ImpCell を確保する．
  ImpCell*
  new_cell();

  /// @brief テーブルの領域を確保する．
  void
  alloc_table(ymuint size);

  /// @brief ハッシュ関数
  ymuint
  hash_func(ymuint src_id,
	    ymuint src_val,
	    ymuint dst_id,
	    ymuint dst_val) const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // ImpCell 用のアロケータ
  UnitAlloc mAlloc;

  // 含意の総数
  ymuint32 mImpNum;

  // mArray のサイズ
  ymuint32 mArraySize;

  // ImpList の配列
  ImpList* mArray;

  // ハッシュ表のサイズ
  ymuint32 mHashSize;

  // ハッシュ表を拡大する目安
  ymuint32 mHashLimit;

  // ハッシュ表
  ImpCell** mHashTable;

};

END_NAMESPACE_YM_NETWORKS

#endif // IMPINFO_H
