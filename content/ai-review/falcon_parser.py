#!/usr/bin/env python3
"""
Falcon 2 Laser Protocol Parser
A comprehensive tool for parsing and analyzing Falcon 2 laser serial communication packets

Usage:
    python3 content/ai-review/falcon_parser.py [dump_file]             # Parse single file
    python3 content/ai-review/falcon_parser.py --csv [dump_file]       # Export as CSV
    python3 content/ai-review/falcon_parser.py --json [dump_file]      # Export as JSON
"""

import struct
import sys
import json
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from collections import defaultdict
from dataclasses import dataclass, asdict

# Big5 character translations for sensor identification
BIG5_TRANSLATIONS = {
    "逼": {
        "bytes": "B9 47",
        "en": "Force/Compel (Max Air)",
        "type": "air_pressure",
        "level": "max",
    },
    "矮": {
        "bytes": "B8 47",
        "en": "Low/Short (Mid Air)",
        "type": "air_pressure",
        "level": "mid",
    },
    "孱": {
        "bytes": "B4 47",
        "en": "Weak/Delicate (Min Air)",
        "type": "air_pressure",
        "level": "min",
    },
    "蛄": {
        "bytes": "B3 47",
        "en": "Mayfly (Idle)",
        "type": "air_pressure",
        "level": "idle",
    },
    "廉": {
        "bytes": "B7 47",
        "en": "Honest (Up/Down Motion)",
        "type": "air_assist",
        "level": "motion",
    },
    "嘮": {
        "bytes": "BC 47",
        "en": "Chat (Fluctuating)",
        "type": "air_assist",
        "level": "fluctuate",
    },
    "慘": {
        "bytes": "BA 47",
        "en": "Miserable (Fire)",
        "type": "fire",
        "level": "alert",
    },
    "腐": {
        "bytes": "BB 47",
        "en": "Decay (Fire Cooling)",
        "type": "fire",
        "level": "cooling",
    },
    "湟": {
        "bytes": "B5 47",
        "en": "Radiance (Fire Proximity)",
        "type": "fire",
        "level": "proximity",
    },
}


@dataclass
class FalconPacket:
    """Parsed Falcon protocol packet"""

    index: int
    msg_type: int
    ctrl1: int
    ctrl2: int
    length: int
    data_hex: str
    data_bytes: bytes

    # Parsed fields
    character: Optional[str] = None
    character_translation: Optional[Dict] = None
    intensity: Optional[int] = None
    float_value: Optional[float] = None
    float_position: Optional[int] = None
    mystery_byte: Optional[int] = None

    def to_dict(self):
        """Convert to dictionary for JSON export"""
        d = asdict(self)
        d["data_bytes"] = self.data_bytes.hex().upper()
        d["character_translation"] = self.character_translation
        return d


class FalconParser:
    """Parse Falcon 2 laser serial packets"""

    def __init__(self):
        self.packets: List[FalconPacket] = []
        self.stats = {
            "total": 0,
            "by_type": defaultdict(int),
            "air_states": defaultdict(int),
            "fire_states": defaultdict(int),
            "float_values": [],
            "mystery_values": defaultdict(int),
        }

    def parse_hex_line(self, hex_line: str) -> Optional[FalconPacket]:
        """Parse a single hex line into a packet"""
        hex_line = hex_line.strip()
        if not hex_line or hex_line.startswith("RX") or hex_line.startswith("TX"):
            return None

        try:
            # Clean up hex string
            hex_clean = hex_line.replace(" ", "")
            packet_bytes = bytes.fromhex(hex_clean)

            if len(packet_bytes) < 10:
                return None

            # Verify prefix and newline
            if packet_bytes[0:4] != bytes([0x49, 0x4C, 0x6D, 0x70]):
                return None
            if packet_bytes[-1] != 0x0A:
                return None

            # Extract header
            length = packet_bytes[4]
            index = packet_bytes[5]
            msg_type = packet_bytes[6]
            ctrl1 = packet_bytes[7]
            ctrl2 = packet_bytes[8]

            # Extract data
            data_start = 9
            data_end = data_start + length
            data = packet_bytes[data_start:data_end]

            packet = FalconPacket(
                index=index,
                msg_type=msg_type,
                ctrl1=ctrl1,
                ctrl2=ctrl2,
                length=length,
                data_hex=data.hex().upper(),
                data_bytes=data,
            )

            # Parse sensor data
            if msg_type == 0x07 and length == 8:
                self._parse_sensor_packet(packet)

            return packet
        except:
            return None

    def _parse_sensor_packet(self, packet: FalconPacket):
        """Parse a sensor data packet (type 0x07)"""
        if len(packet.data_bytes) < 8:
            return

        # Extract fields
        packet.intensity = packet.data_bytes[0]

        # Try to decode Big5 character
        big5_byte1 = packet.data_bytes[1]
        big5_byte2 = packet.data_bytes[2]

        if big5_byte1 != 0x00:
            try:
                char = bytes([big5_byte1, big5_byte2]).decode("big5")
                packet.character = char
                packet.character_translation = BIG5_TRANSLATIONS.get(char)
            except:
                pass

        # Extract IEEE float (try multiple positions)
        for start, end in [(2, 6), (3, 7), (4, 8)]:
            try:
                float_bytes = packet.data_bytes[start:end]
                if len(float_bytes) == 4:
                    value = struct.unpack("<f", float_bytes)[0]
                    # Check if valid float
                    if not (
                        value != value
                        or value == float("inf")
                        or value == float("-inf")
                    ):
                        packet.float_value = value
                        packet.float_position = start
                        break
            except:
                pass

        # Extract mystery byte
        packet.mystery_byte = packet.data_bytes[7]

        # Update stats
        if packet.character:
            trans = packet.character_translation
            if trans and trans["type"] == "air_pressure":
                self.stats["air_states"][trans["level"]] += 1
            elif trans and trans["type"] == "fire":
                self.stats["fire_states"][trans["level"]] += 1

        if packet.float_value:
            self.stats["float_values"].append(packet.float_value)

        self.stats["mystery_values"][packet.mystery_byte] += 1

    def parse_file(self, filepath: Path) -> List[FalconPacket]:
        """Parse a dump file"""
        self.packets = []
        print(f"Parsing {filepath.name}...")

        with open(filepath, "r") as f:
            for line in f:
                packet = self.parse_hex_line(line)
                if packet:
                    self.packets.append(packet)
                    self.stats["total"] += 1
                    self.stats["by_type"][packet.msg_type] += 1

        print(f"  Loaded {len(self.packets)} packets")
        return self.packets

    def print_summary(self):
        """Print summary statistics"""
        print("\n" + "=" * 80)
        print("PACKET STATISTICS")
        print("=" * 80)
        print(f"Total packets: {self.stats['total']}")

        print(f"\nBy Type:")
        for msg_type in sorted(self.stats["by_type"].keys()):
            count = self.stats["by_type"][msg_type]
            type_names = {
                0x00: "Keep-Alive",
                0x01: "Header/Model",
                0x03: "Status",
                0x07: "Sensor Data",
            }
            print(
                f"  0x{msg_type:02X} ({type_names.get(msg_type, 'Unknown')}): {count}"
            )

        if self.stats["air_states"]:
            print(f"\nAir Pressure States:")
            for level in ["max", "mid", "min", "idle"]:
                if level in self.stats["air_states"]:
                    print(f"  {level.upper()}: {self.stats['air_states'][level]}")

        if self.stats["fire_states"]:
            print(f"\nFire States:")
            for level in ["alert", "cooling", "proximity"]:
                if level in self.stats["fire_states"]:
                    print(f"  {level.upper()}: {self.stats['fire_states'][level]}")

        if self.stats["float_values"]:
            floats = self.stats["float_values"]
            print(f"\nCalibration Float Values (0x07 packets):")
            print(f"  Count: {len(floats)}")
            print(f"  Min: {min(floats):.2f}")
            print(f"  Max: {max(floats):.2f}")
            print(f"  Mean: {sum(floats)/len(floats):.2f}")

        if self.stats["mystery_values"]:
            print(f"\nMystery Byte Distribution (last byte of 0x07 data):")
            for byte_val in sorted(self.stats["mystery_values"].keys()):
                count = self.stats["mystery_values"][byte_val]
                print(f"  0x{byte_val:02X}: {count} times")

    def print_samples(self, msg_type: int = 0x07, count: int = 5):
        """Print sample packets of a given type"""
        samples = [p for p in self.packets if p.msg_type == msg_type][:count]

        type_names = {
            0x00: "Keep-Alive",
            0x01: "Header",
            0x03: "Status",
            0x07: "Sensor Data",
        }
        print(
            f"\n{type_names.get(msg_type, f'Type 0x{msg_type:02X}')} Packets (showing {len(samples)} samples):"
        )
        print("-" * 80)

        for i, packet in enumerate(samples, 1):
            print(
                f"\n{i}. Index: 0x{packet.index:02X} | Type: 0x{packet.msg_type:02X} | Length: {packet.length}"
            )
            print(f"   Data: {packet.data_hex}")

            if packet.msg_type == 0x07:
                print(f"   Parsed:")
                print(f"     Intensity: 0x{packet.intensity:02X} ({packet.intensity})")
                if packet.character:
                    trans = packet.character_translation
                    if trans:
                        print(f"     Sensor: {packet.character} - {trans['en']}")
                    else:
                        print(f"     Character: {packet.character}")
                if packet.float_value is not None:
                    print(
                        f"     Calibration: {packet.float_value:.2f} (at position {packet.float_position})"
                    )
                if packet.mystery_byte is not None:
                    print(f"     Mystery Byte: 0x{packet.mystery_byte:02X}")

    def export_json(self, filepath: Path):
        """Export packets to JSON"""
        data = {
            "filename": filepath.name,
            "total_packets": len(self.packets),
            "statistics": dict(self.stats),
            "packets": [
                p.to_dict() for p in self.packets[:100]
            ],  # Limit to 100 for file size
        }

        json_file = filepath.with_suffix(".json")
        with open(json_file, "w") as f:
            json.dump(data, f, indent=2)
        print(f"\nExported to {json_file}")

    def export_csv(self, filepath: Path):
        """Export sensor packets to CSV"""
        sensor_packets = [p for p in self.packets if p.msg_type == 0x07]

        csv_file = filepath.with_name(filepath.stem + "_sensors.csv")
        with open(csv_file, "w") as f:
            # Write header
            f.write(
                "Index,Intensity,Character,Sensor_Type,Calibration,Mystery_Byte,Data_Hex\n"
            )

            for p in sensor_packets:
                char = p.character or ""
                sensor_type = (
                    p.character_translation.get("type", "")
                    if p.character_translation
                    else ""
                )
                calib = f"{p.float_value:.2f}" if p.float_value else ""
                mystery = (
                    f"0x{p.mystery_byte:02X}" if p.mystery_byte is not None else ""
                )

                f.write(
                    f"0x{p.index:02X},0x{p.intensity:02X},{char},{sensor_type},{calib},{mystery},{p.data_hex}\n"
                )

        print(f"Exported {len(sensor_packets)} sensor packets to {csv_file}")


def main():
    if len(sys.argv) < 2:
        print("Falcon 2 Laser Protocol Parser")
        print("\nUsage:")
        print(
            "  python3 content/ai-review/falcon_parser.py <dump_file>           - Parse and display packet info"
        )
        print(
            "  python3 content/ai-review/falcon_parser.py --csv <dump_file>     - Export sensor data to CSV"
        )
        print(
            "  python3 content/ai-review/falcon_parser.py --json <dump_file>    - Export data to JSON"
        )
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == "--csv" and len(sys.argv) > 2:
        filepath = Path(sys.argv[2])
        parser = FalconParser()
        parser.parse_file(filepath)
        parser.print_summary()
        parser.export_csv(filepath)

    elif cmd == "--json" and len(sys.argv) > 2:
        filepath = Path(sys.argv[2])
        parser = FalconParser()
        parser.parse_file(filepath)
        parser.print_summary()
        parser.export_json(filepath)

    else:
        filepath = Path(cmd)
        if not filepath.exists():
            print(f"Error: {filepath} not found")
            sys.exit(1)

        parser = FalconParser()
        parser.parse_file(filepath)
        parser.print_summary()
        parser.print_samples(0x07, count=3)
        parser.print_samples(0x01, count=1)


if __name__ == "__main__":
    main()
