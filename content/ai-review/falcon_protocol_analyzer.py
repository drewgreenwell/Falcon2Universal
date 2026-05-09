#!/usr/bin/env python3
"""
Falcon 2 Laser Protocol Analyzer
Analyzes hex dumps to decode mystery field, extract calibration values, and build protocol documentation
"""

import struct
import glob
from pathlib import Path
from typing import List, Dict, Tuple
from collections import defaultdict


class FalconPacket:
    """Parse and analyze a single Falcon protocol packet"""

    def __init__(self, hex_str: str):
        self.hex_str = hex_str.strip()
        self.bytes = bytes.fromhex(hex_str.replace(" ", ""))
        self.parse()

    def parse(self):
        """Extract packet fields"""
        if len(self.bytes) < 10:
            raise ValueError(f"Packet too short: {len(self.bytes)} bytes")

        self.prefix = self.bytes[0:4]
        self.length = self.bytes[4]
        self.index = self.bytes[5]
        self.msg_type = self.bytes[6]
        self.ctrl1 = self.bytes[7]
        self.ctrl2 = self.bytes[8]

        # Data payload
        data_start = 9
        data_end = data_start + self.length
        self.data = self.bytes[data_start:data_end]

        # Verify newline at end
        if self.bytes[-1] != 0x0A:
            raise ValueError(f"Packet doesn't end with newline: {self.bytes[-1]:02X}")

        # Mystery field is the byte before newline (if it exists)
        # Structure: PREFIX(4) + LEN(1) + IDX(1) + TYPE(1) + CTRL1(1) + CTRL2(1) + DATA(N) + [MYSTERY(1)?] + NEWLINE(1)
        # Total positions: 0-3(prefix) 4(len) 5(idx) 6(type) 7(ctrl1) 8(ctrl2) 9 to 9+N-1(data) data_end onwards
        # If packet has extra byte before 0x0A, it's at position data_end

        if len(self.bytes) >= data_end + 2:  # Mystery byte + newline after data
            self.mystery = self.bytes[data_end]
        else:
            self.mystery = None

    def decode_big5_char(self) -> str:
        """Decode Big5 encoded Chinese character from data"""
        if self.length == 0 or len(self.data) == 0:
            return None

        # For 8-byte payloads with sensor data:
        # data[0] = intensity modifier
        # data[1] = first byte of Big5 (may be 0x00 if not used)
        # data[2] = second byte of Big5
        if self.length >= 3 and len(self.data) >= 3:
            byte1 = self.data[1]
            byte2 = self.data[2]

            # Only decode if first byte is non-zero (indicates Big5 character)
            if byte1 != 0x00:
                try:
                    char = bytes([byte1, byte2]).decode("big5")
                    return char
                except:
                    return None

        return None

    def extract_ieee_float(self) -> Tuple[float, int]:
        """Extract IEEE 32-bit float and try different positions, return (float, position)"""
        # For 8-byte payloads, try positions 2-5, 3-6, 4-7
        if self.length < 8 or len(self.data) < 8:
            return None, None

        # Common structure: intensity(1) + Big5(2) + padding/calib(2) + float(4)
        # But we have 8 bytes total, so maybe: intensity(1) + Big5(2) + float(4) + extra(1)

        positions = [(2, 6), (3, 7), (4, 8)]

        for start, end in positions:
            try:
                float_bytes = self.data[start:end]
                if len(float_bytes) == 4:
                    value = struct.unpack("<f", float_bytes)[0]
                    # Only return if it's a reasonable sensor value (not NaN/Inf)
                    if not (
                        value != value
                        or value == float("inf")
                        or value == float("-inf")
                    ):
                        return value, start
            except:
                pass

        return None, None

    def __repr__(self):
        char = self.decode_big5_char()
        float_val, float_pos = self.extract_ieee_float()
        mystery_str = f"{self.mystery:02X}" if self.mystery else "None"
        float_str = f"{float_val:.2f}" if float_val is not None else "None"
        return f"Packet(idx={self.index:02X}, type={self.msg_type:02X}, len={self.length}, char={char}, float={float_str}, mystery={mystery_str})"


class FalconDumpAnalyzer:
    """Analyze multiple dump files to understand the protocol"""

    def __init__(self, dump_dir: str):
        self.dump_dir = Path(dump_dir)
        self.packets: List[FalconPacket] = []
        self.mystery_stats = defaultdict(list)
        self.character_stats = defaultdict(list)
        self.float_stats = defaultdict(list)

    def load_dumps(self):
        """Load all dump files from directory"""
        print(f"Loading dumps from {self.dump_dir}")

        # Find both RX and TX dumps
        dump_files = list(self.dump_dir.glob("**/*rx.txt")) + list(
            self.dump_dir.glob("**/*tx.txt")
        )
        dump_files = sorted(dump_files)

        print(f"Found {len(dump_files)} dump files")

        for dump_file in dump_files[:5]:  # Limit to first 5 for initial analysis
            self.load_file(dump_file)

    def load_file(self, filepath: Path):
        """Load and parse a single dump file"""
        print(f"  Processing {filepath.name}...", end=" ")

        with open(filepath, "r") as f:
            lines = f.readlines()

        count = 0
        for line in lines:
            try:
                packet = FalconPacket(line)
                self.packets.append(packet)
                count += 1

                # Collect statistics
                if packet.msg_type == 0x07:  # Sensor data
                    char = packet.decode_big5_char()
                    float_val, float_pos = packet.extract_ieee_float()

                    if packet.mystery is not None:
                        self.mystery_stats[packet.mystery].append(
                            {
                                "packet": packet,
                                "char": char,
                                "float": float_val,
                                "file": filepath.name,
                            }
                        )

                    if char:
                        self.character_stats[char].append(
                            {
                                "packet": packet,
                                "float": float_val,
                                "file": filepath.name,
                            }
                        )
            except Exception as e:
                pass  # Skip malformed packets

        print(f"{count} packets")

    def analyze_mystery_field(self):
        """Analyze patterns in the mystery field"""
        print("\n" + "=" * 80)
        print("MYSTERY FIELD ANALYSIS (Byte before 0x0A)")
        print("=" * 80)

        mystery_values = sorted(self.mystery_stats.keys())
        print(f"\nFound {len(mystery_values)} unique mystery field values:")

        for mystery_byte in mystery_values:
            entries = self.mystery_stats[mystery_byte]
            chars = set(e["char"] for e in entries if e["char"])
            floats = [e["float"] for e in entries if e["float"] is not None]

            avg_float = sum(floats) / len(floats) if floats else None
            min_float = min(floats) if floats else None
            max_float = max(floats) if floats else None

            print(
                f"\n  0x{mystery_byte:02X} (binary: {mystery_byte:08b}, decimal: {mystery_byte})"
            )
            print(f"    Count: {len(entries)}")
            print(f"    Characters seen: {chars}")
            if avg_float is not None:
                print(
                    f"    Float range: {min_float:.4f} to {max_float:.4f} (avg: {avg_float:.4f})"
                )

    def analyze_floats(self):
        """Analyze IEEE float calibration values"""
        print("\n" + "=" * 80)
        print("IEEE FLOAT CALIBRATION ANALYSIS")
        print("=" * 80)

        all_floats = defaultdict(list)

        for packet in self.packets:
            if packet.msg_type == 0x07:
                char = packet.decode_big5_char()
                float_val, float_pos = packet.extract_ieee_float()

                if char and float_val is not None:
                    all_floats[char].append(float_val)

        print(f"\nFound {len(all_floats)} unique characters with float data:")

        for char in sorted(all_floats.keys()):
            floats = all_floats[char]
            print(f"\n  '{char}' (Big5: {char.encode('big5').hex().upper()})")
            print(f"    Sample values: {floats[:5]}")
            print(f"    Range: {min(floats):.4f} to {max(floats):.4f}")
            print(f"    Mean: {sum(floats)/len(floats):.4f}")

    def print_packet_samples(self):
        """Print sample packets of each type"""
        print("\n" + "=" * 80)
        print("SAMPLE PACKETS BY TYPE")
        print("=" * 80)

        by_type = defaultdict(list)
        for packet in self.packets:
            by_type[packet.msg_type].append(packet)

        for msg_type in sorted(by_type.keys()):
            packets = by_type[msg_type]
            print(f"\nType 0x{msg_type:02X} ({len(packets)} packets):")

            for packet in packets[:2]:
                print(f"  {packet}")
                print(f"    Raw data: {packet.data.hex().upper()}")
                print(f"    Data breakdown:")

                if packet.length == 8:
                    print(f"      [0] Intensity: 0x{packet.data[0]:02X}")
                    if len(packet.data) >= 3:
                        char = packet.decode_big5_char()
                        print(
                            f"      [1-2] Big5 char: {char} (0x{packet.data[1]:02X} {packet.data[2]:02X})"
                        )
                    if len(packet.data) >= 5:
                        print(
                            f"      [3-4] Calibration: 0x{packet.data[3]:02X} 0x{packet.data[4]:02X}"
                        )
                    if len(packet.data) >= 8:
                        float_val, float_pos = packet.extract_ieee_float()
                        print(f"      Float at position [{float_pos}]: {float_val}")


def main():
    # Resolve repo root from this script location: <repo>/content/ai-review/
    repo_root = Path(__file__).resolve().parents[2]
    dump_dir = repo_root / "content/3.0.5-Dumps"

    analyzer = FalconDumpAnalyzer(str(dump_dir))
    analyzer.load_dumps()

    print(f"\nTotal packets loaded: {len(analyzer.packets)}")

    analyzer.print_packet_samples()
    analyzer.analyze_mystery_field()
    analyzer.analyze_floats()


if __name__ == "__main__":
    main()
