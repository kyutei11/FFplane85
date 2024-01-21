# FFplane85
ATtiny85を用いた
- モーターラン
- デサマライザー信号発信

を行うプログラムです。

モーターラン時間とデサマライザー作動時刻は起動時に設定できます。

設定方法：
- スイッチを500mSec以上長押しで設定に移行
- スイッチを短時間押すごとにモーターラン時間が5,10,15...秒増加
   - 押すごとにLEDが最小単位の倍数分短時間点滅，プログラムで設定の最大値を超えると最小値に戻る
- スイッチを長押しでデサマライザー作動時刻設定に移行
- スイッチを短時間押すごとに作動時刻が10,20,30...秒（モーターラン時間より短ければ自動的にモーターラン時間と同じとなる）
   - 押すごとにLEDが最小単位の倍数分短時間"2回"点滅，プログラムで設定の最大値を超えると最小値に戻る
- スイッチを長押しでEEPROMに設定書き込み：フライトモードに移行

起動後にスイッチ長押しでなくスイッチを短く押すとEEPROMの設定を読み込んでフライトモードに移行します。

フライトモードでボタンを押すとデサマライザーが起動するとともにLEDが1秒毎に短時間点滅し，5秒後にモーターが起動します。

プログラムで設定した電圧以下で起動した場合にはLEDが1s周期で点滅したままフライトモードに移行しません。
また，フライトモードで設定電圧以下となった場合にもモーターは停止します。

このコードで対応するモーターは小型のコアレスモータを想定しています。が，あくまでPWMコマンドですのでブラシDCモータとそれに対応するFETであれば対応可能です。
電池電圧に関係なくモーター実効電圧が一定となるようにPWMデューティー比を調整するプログラムとしています。

各設定時間単位と使用モーター電圧はプログラム中で調整してください。

LEDのポートはデバッグ用の送信専用簡易シリアルポート（2400bps）も兼ねています。

Copyright (C) 2023− Koichi Takasaki

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

