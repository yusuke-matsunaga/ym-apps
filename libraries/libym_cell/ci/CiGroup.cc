
/// @file CiGroup.cc
/// @brief CiGroup の実装ファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2011 Yusuke Matsunaga
/// All rights reserved.


#include "CiGroup.h"

#include "ym_cell/CellLibrary.h"
#include "ym_cell/CellClass.h"
#include "ym_cell/Cell.h"


BEGIN_NAMESPACE_YM_CELL

BEGIN_NONAMESPACE

const int OUTPUT1 = 0;
const int OUTPUT2 = 1;
const int INPUT   = 2;
const int CLOCK   = 3;
const int ENABLE  = 3;
const int CLEAR   = 4;
const int PRESET  = 5;

inline
ymuint32
encode(ymuint val,
       int idx)
{
  return val << (5 * idx);
}

inline
ymuint
get_sense(ymuint32 bits,
	  int idx)
{
  return (bits >> (5 * idx + 3)) & 3U;
}

inline
ymuint
get_pos(ymuint32 bits,
	int idx)
{
  return (bits >> (5 * idx)) & 7U;
}

END_NONAMESPACE

//////////////////////////////////////////////////////////////////////
// クラス CiGroup
//////////////////////////////////////////////////////////////////////

// @brief コンストラクタ
CiGroup::CiGroup() :
  mCellClass(NULL),
  mPinInfo(0U),
  mCellNum(0),
  mCellList(NULL)
{
}

// @brief デストラクタ
CiGroup::~CiGroup()
{
  // mCellList は CellMgr が管理する．
}

// @brief ID番号を返す．
// @note CellLibrary::group(id) で返されるオブジェクトの id() は id となる．
ymuint
CiGroup::id() const
{
  return mId;
}

// @brief 属している CellClass を返す．
const CellClass*
CiGroup::cell_class() const
{
  return mCellClass;
}

// @brief 代表クラスに対する変換マップを返す．
const NpnMapM&
CiGroup::map() const
{
  return mMap;
}

// @brief データ入力を持つとき true を返す．
bool
CiGroup::has_data() const
{
  return get_sense(mPinInfo, INPUT) != 0;
}

// @brief データ入力のピン番号を返す．
ymuint
CiGroup::data_pos() const
{
  return get_pos(mPinInfo, INPUT);
}

// @brief クロック入力のタイプを返す．
// @retval 0 該当しない
// @retval 1 positive edge
// @retval 2 negative edge
ymuint
CiGroup::clock_sense() const
{
  return get_sense(mPinInfo, CLOCK);
}

// @brief クロック入力のピン番号を返す．
ymuint
CiGroup::clock_pos() const
{
  return get_pos(mPinInfo, CLOCK);
}

// @brief イネーブル入力を持つとき true を返す．
bool
CiGroup::has_enable() const
{
  return enable_sense() != 0;
}

// @brief イネーブル入力のタイプを返す．
// @retval 0 なし
// @retval 1 positive edge
// @retval 2 negative edge
ymuint
CiGroup::enable_sense() const
{
  return get_sense(mPinInfo, ENABLE);
}

// @brief イネーブル入力のピン番号を返す．
ymuint
CiGroup::enable_pos() const
{
  return get_pos(mPinInfo, ENABLE);
}

// @brief クリア入力を持つタイプの時に true を返す．
bool
CiGroup::has_clear() const
{
  return clear_sense() != 0;
}

// @brief クリア入力のタイプを返す．
// @retval 0 なし
// @retval 1 High sensitive
// @retval 2 Low sensitive
ymuint
CiGroup::clear_sense() const
{
  return get_sense(mPinInfo, CLEAR);
}

// @brief クリア入力のピン番号を返す．
// @note クリア入力がない場合の値は不定
ymuint
CiGroup::clear_pos() const
{
  return get_pos(mPinInfo, CLEAR);
}

// @brief プリセット入力を持つタイプの時に true を返す．
bool
CiGroup::has_preset() const
{
  return preset_sense() != 0;
}

// @brief プリセット入力のタイプを返す．
// @retval 0 なし
// @retval 1 High sensitive
// @retval 2 Low sensitive
ymuint
CiGroup::preset_sense() const
{
  return get_sense(mPinInfo, PRESET);
}

// @brief プリセット入力のピン番号を返す．
// @note プリセット入力がない場合の値は不定
ymuint
CiGroup::preset_pos() const
{
  return get_pos(mPinInfo, PRESET);
}

// @brief 肯定出力のピン番号を返す．
ymuint
CiGroup::q_pos() const
{
  return get_pos(mPinInfo, OUTPUT1);
}

// @brief 否定出力のピン番号を返す．
ymuint
CiGroup::xq_pos() const
{
  return get_pos(mPinInfo, OUTPUT2);
}

// @brief セル数を返す．
ymuint
CiGroup::cell_num() const
{
  return mCellNum;
}

// @brief セルを返す．
// @param[in] pos 位置番号 ( 0 <= pos < cell_num() )
const Cell*
CiGroup::cell(ymuint pos) const
{
  assert_cond( pos < mCellNum, __FILE__, __LINE__);
  return mCellList[pos];
}

// @brief 初期化する．
// @param[in] cell_class 代表クラス
// @param[in] map 変換マップ
// @param[in] cell_list セルのリスト
// @param[in] alloc メモリアロケータ
void
CiGroup::init(const CellClass* cell_class,
	      const NpnMapM& map,
	      const vector<const Cell*>& cell_list,
	      AllocBase& alloc)
{
  mCellClass = cell_class;
  mMap = map;
  mCellNum = cell_list.size();

  alloc_array(alloc);
  for (ymuint i = 0; i < mCellNum; ++ i) {
    mCellList[i] = cell_list[i];
  }
}

// @brief FFのピン情報を設定する．
// @param[in] pos_array ピン位置と極性情報の配列
// @note pos_array の意味は以下の通り
//  - pos_array[0] : データ入力のピン番号     (3bit)
//  - pos_array[1] : クロック入力のピン番号   (3bit) | 極性情報 (1bit)
//  - pos_array[2] : クリア入力のピン番号     (3bit) | 極性情報 (2bit)
//  - pos_array[3] : プリセット入力のピン番号 (3bit) | 極性情報 (2bit)
//  - pos_array[4] : 肯定出力のピン番号       (3bit)
//  - pos_array[5] : 否定出力のピン番号       (3bit)
void
CiGroup::set_ff_info(ymuint pos_array[])
{
  mPinInfo = 0U;
  mPinInfo |= encode(pos_array[0], INPUT);
  mPinInfo |= encode(pos_array[1], CLOCK);
  mPinInfo |= encode(pos_array[2], CLEAR);
  mPinInfo |= encode(pos_array[3], PRESET);
  mPinInfo |= encode(pos_array[4], OUTPUT1);
  mPinInfo |= encode(pos_array[5], OUTPUT2);
}

// @brief ラッチのピン情報を設定する．
// @param[in] pos_array ピン位置と極性情報の配列
// @note pos_array の意味は以下の通り
//  - pos_array[0] : データ入力のピン番号     (3bit)
//  - pos_array[1] : イネーブル入力のピン番号 (3bit) | 極性情報 (2bit)
//  - pos_array[2] : クリア入力のピン番号     (3bit) | 極性情報 (2bit)
//  - pos_array[3] : プリセット入力のピン番号 (3bit) | 極性情報 (2bit)
//  - pos_array[4] : 肯定出力のピン番号       (3bit)
//  - pos_array[5] : 否定出力のピン番号       (3bit)
void
CiGroup::set_latch_info(ymuint pos_array[])
{
  mPinInfo = 0U;
  mPinInfo |= encode(pos_array[0], INPUT);
  mPinInfo |= encode(pos_array[1], ENABLE);
  mPinInfo |= encode(pos_array[2], CLEAR);
  mPinInfo |= encode(pos_array[3], PRESET);
  mPinInfo |= encode(pos_array[4], OUTPUT1);
}

// @brief バイナリダンプを行う．
// @param[in] bos 出力先のストリーム
void
CiGroup::dump(BinO& bos) const
{
  ymuint32 parent_id = mCellClass->id();
  bos << parent_id
      << mMap
      << mPinInfo
      << mCellNum;
  for (ymuint i = 0; i < mCellNum; ++ i) {
    ymuint32 cell_id = mCellList[i]->id();
    bos << cell_id;
  }
}

// @brief バイナリファイルを読み込む．
// @param[in] bis 入力元のストリーム
// @param[in] library セルライブラリ
// @param[in] alloc メモリアロケータ
void
CiGroup::restore(BinI& bis,
		 const CellLibrary& library,
		 AllocBase& alloc)
{
  ymuint32 parent_id;
  bis >> parent_id
      >> mMap
      >> mPinInfo
      >> mCellNum;
  mCellClass = library.npn_class(parent_id);

  alloc_array(alloc);
  for (ymuint i = 0; i < mCellNum; ++ i) {
    ymuint32 cell_id;
    bis >> cell_id;
    mCellList[i] = library.cell(cell_id);
  }
}

// @brief メモリ領域の確保を行う．
// @param[in] alloc メモリアロケータ
void
CiGroup::alloc_array(AllocBase& alloc)
{
  if ( mCellNum > 0 ) {
    void* p = alloc.get_memory(sizeof(const Cell*) * mCellNum);
    mCellList = new (p) const Cell*[mCellNum];
  }
  else {
    mCellList = NULL;
  }
}

END_NAMESPACE_YM_CELL
