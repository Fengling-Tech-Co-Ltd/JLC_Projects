# -*- coding: utf-8 -*-
"""模块插槽小板 实时监视（SWD 轮询，不占用串口）

每轮 halt -> 读 RAM -> resume，读的是固件里的寄存器组：
  s_pos  : board_id / slot_id / position / adc_raw / valid
  s_regs : position / ids / status / temp(0.1℃, int16) / ntc_state / adc

用法：
  python ntc_monitor.py              # 一直刷，Ctrl+C 退出
  python ntc_monitor.py --count 5    # 只刷 5 次，用于自检
"""
import argparse
import os
import re
import sys
import time

from pyocd.core.helpers import ConnectHelper

UID = "070000814c57445f113134303934554ba5a5a5a597969908"
TARGET = "py32f002ax5"
FREQ = 100000
PACK = r"H:/Keil_5_MDK524/STM32/ARM/PACK/.Download/Puya.PY32F0xx_DFP.1.2.5.pack"

MAP_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "MDK-ARM", "Output", "Project.map")

NTC_STATE = {0: "OK", 1: "OPEN", 2: "SHORT", 3: "I2CERR"}


def map_addr(symbol):
    """从 Keil 的 .map 里取符号地址，固件改了不用手动改脚本。"""
    with open(MAP_PATH, encoding="utf-8", errors="ignore") as fh:
        text = fh.read()
    m = re.search(r"\b%s\b\s+0x([0-9a-fA-F]{8})" % re.escape(symbol), text)
    if not m:
        raise SystemExit("map 里找不到符号 %s，先编译一次" % symbol)
    return int(m.group(1), 16)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--count", type=int, default=0, help="刷几次，0=一直刷")
    ap.add_argument("--interval", type=float, default=1.0, help="刷新间隔（秒）")
    args = ap.parse_args()

    addr_pos = map_addr("s_pos")
    addr_regs = map_addr("s_regs")
    print("s_pos=0x%08X  s_regs=0x%08X" % (addr_pos, addr_regs))

    session = ConnectHelper.session_with_chosen_probe(
        unique_id=UID, target_override=TARGET,
        options={"frequency": FREQ, "pack": PACK})
    if session is None:
        raise SystemExit("连不上探针 %s" % UID)

    n = 0
    try:
        with session:
            target = session.target
            print("%-10s %-4s %4s %-7s %8s %5s" %
                  ("时间", "板", "槽位", "NTC", "温度", "液位"))
            print("-" * 48)
            while True:
                target.halt()
                try:
                    p = target.read_memory_block8(addr_pos, 8)
                    r = target.read_memory_block8(addr_regs, 8)
                finally:
                    target.resume()

                board_id, slot_id, position = p[0], p[1], p[2]
                adc_raw = p[4] | (p[5] << 8)
                valid = p[6]

                state = NTC_STATE.get(r[5], "?")
                temp = (r[3] | (r[4] << 8))
                if temp >= 0x8000:
                    temp -= 0x10000
                temp_s = "%.1f" % (temp / 10.0) if state == 0 else "--"
                level = "湿" if (r[2] & 0x01) else "干"

                # board_id / slot_id 都是 0 基（板一=0, CN1=0），显示统一换成 1 基：
                #   板1~板4、槽位1~4。括号里保留原始 0 基值便于对照寄存器。
                print("%-10s %-4s %4d %-7s %8s %5s   (board_id=%d slot_id=%d adc=%d valid=%d)" %
                      (time.strftime("%H:%M:%S"), "板%d" % (board_id + 1), slot_id + 1,
                       state, temp_s, level, board_id, slot_id, adc_raw, valid))

                n += 1
                if args.count and n >= args.count:
                    break
                time.sleep(args.interval)
    except KeyboardInterrupt:
        pass
    except Exception as exc:                      # noqa: BLE001
        print("监视中断：%s" % exc)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
