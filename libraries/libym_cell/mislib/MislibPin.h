#ifndef LIBYM_CELL_MISLIB_MISLIBPIN_H
#define LIBYM_CELL_MISLIB_MISLIBPIN_H

/// @file libym_cell/mislib/MislibPin.h
/// @brief MislibPin のヘッダファイル
/// @author Yusuke Matsunaga (松永 裕介)
///
/// Copyright (C) 2005-2010 Yusuke Matsunaga
/// All rights reserved.


#include "ym_cell/CellPin.h"
#include "ym_utils/ShString.h"
#include "ym_lexp/LogExpr.h"


BEGIN_NAMESPACE_YM_CELL

//////////////////////////////////////////////////////////////////////
/// @class MislibPin MislibPin.h "MislibPin.h"
/// @brief Mislib 用の CellPin の実装クラス
//////////////////////////////////////////////////////////////////////
class MislibPin :
  public CellPin
{
  friend class MislibLibrary;

protected:

  /// @brief コンストラクタ
  MislibPin();

  /// @brief デストラクタ
  virtual
  ~MislibPin();


public:
  //////////////////////////////////////////////////////////////////////
  // 共通属性
  //////////////////////////////////////////////////////////////////////

  /// @brief ピン名を返す．
  virtual
  string
  name() const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // 名前
  ShString mName;

};


//////////////////////////////////////////////////////////////////////
/// @class MislibInputPin MislibPin.h "MislibPin.h"
/// @brief 入力ピンを表す MislibPin の継承クラス
//////////////////////////////////////////////////////////////////////
class MislibInputPin :
  public MislibPin
{
  friend class MislibLibrary;

private:

  /// @brief コンストラクタ
  MislibInputPin();

  /// @brief デストラクタ
  virtual
  ~MislibInputPin();


public:
  //////////////////////////////////////////////////////////////////////
  // 共通属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 方向を返す．
  virtual
  tCellDirection
  direction() const;


public:
  //////////////////////////////////////////////////////////////////////
  // 入力ピンの属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 負荷容量を返す．
  virtual
  CellCapacitance
  capacitance() const;

  /// @brief 立ち上がり時の負荷容量を返す．
  virtual
  CellCapacitance
  rise_capacitance() const;

  /// @brief 立ち下がり時の負荷容量を返す．
  virtual
  CellCapacitance
  fall_capacitance() const;


public:
  //////////////////////////////////////////////////////////////////////
  // 出力ピンの属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 機能を表す論理式を返す．
  virtual
  LogExpr
  function() const;

  /// @brief 最大ファンアウト容量を返す．
  virtual
  CellCapacitance
  max_fanout() const;

  /// @brief 最小ファンアウト容量を返す．
  virtual
  CellCapacitance
  min_fanout() const;

  /// @brief 最大負荷容量を返す．
  virtual
  CellCapacitance
  max_capacitance() const;

  /// @brief 最小負荷容量を返す．
  virtual
  CellCapacitance
  min_capacitance() const;

  /// @brief 最大遷移時間を返す．
  virtual
  CellTime
  max_transition() const;

  /// @brief 最小遷移時間を返す．
  virtual
  CellTime
  min_transition() const;

  /// @brief タイミング情報の取得
  /// @param[in] ipos 入力ピン番号
  /// @param[out] timing_list タイミング情報を納めるベクタ
  /// @return 条件に合致するタイミング情報の数を返す．
  virtual
  ymuint
  timing(ymuint ipos,
	 vector<const CellTiming*>& timing_list) const;

  /// @brief タイミング情報の取得
  /// @param[in] ipos 開始ピン番号
  /// @param[in] timing_sense タイミング情報の摘要条件
  /// @return 条件に合致するタイミング情報を返す．
  /// @note なければ NULL を返す．
  virtual
  const CellTiming*
  timing(ymuint ipos,
	 tCellTimingSense timing_sense) const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // 負荷容量
  CellCapacitance mCapacitance;

};


//////////////////////////////////////////////////////////////////////
/// @class MislibOutputPin MislibPin.h "MislibPin.h"
/// @brief 出力ピン用の MislibPin の継承クラス
//////////////////////////////////////////////////////////////////////
class MislibOutputPin :
  public MislibPin
{
  friend class MislibLibrary;
  friend class MislibCell;

private:

  /// @brief コンストラクタ
  MislibOutputPin();

  /// @brief デストラクタ
  virtual
  ~MislibOutputPin();


public:
  //////////////////////////////////////////////////////////////////////
  // 共通属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 方向を返す．
  virtual
  tCellDirection
  direction() const;


public:
  //////////////////////////////////////////////////////////////////////
  // 入力ピンの属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 負荷容量を返す．
  virtual
  CellCapacitance
  capacitance() const;

  /// @brief 立ち上がり時の負荷容量を返す．
  virtual
  CellCapacitance
  rise_capacitance() const;

  /// @brief 立ち下がり時の負荷容量を返す．
  virtual
  CellCapacitance
  fall_capacitance() const;


public:
  //////////////////////////////////////////////////////////////////////
  // 出力ピンの属性
  //////////////////////////////////////////////////////////////////////

  /// @brief 機能を表す論理式を返す．
  virtual
  LogExpr
  function() const;

  /// @brief 最大ファンアウト容量を返す．
  virtual
  CellCapacitance
  max_fanout() const;

  /// @brief 最小ファンアウト容量を返す．
  virtual
  CellCapacitance
  min_fanout() const;

  /// @brief 最大負荷容量を返す．
  virtual
  CellCapacitance
  max_capacitance() const;

  /// @brief 最小負荷容量を返す．
  virtual
  CellCapacitance
  min_capacitance() const;

  /// @brief 最大遷移時間を返す．
  virtual
  CellTime
  max_transition() const;

  /// @brief 最小遷移時間を返す．
  virtual
  CellTime
  min_transition() const;

  /// @brief タイミング情報の取得
  /// @param[in] ipos 開始ピン番号
  /// @param[in] sense タイミング情報の摘要条件 ( kPosiUnate か kNegaUnate )
  /// @return 条件に合致するタイミング情報を返す．
  /// @note なければ NULL を返す．
  virtual
  const CellTiming*
  timing(ymuint ipos,
	 tCellTimingSense sense) const;


private:
  //////////////////////////////////////////////////////////////////////
  // データメンバ
  //////////////////////////////////////////////////////////////////////

  // 機能を表す論理式
  LogExpr mFunction;

  // タイミング情報の配列
  // サイズは入力ピン数 x 2
  CellTiming** mTimingArray;

};

END_NAMESPACE_YM_CELL

#endif // LIBYM_CELL_MISLIB_MISLIBPIN_H
