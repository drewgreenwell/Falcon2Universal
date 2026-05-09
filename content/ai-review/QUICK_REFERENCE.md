# Falcon 2 Protocol - Quick Reference

## Packet Structure at a Glance
```
49 4C 6D 70 | LL | II | TT | C1 | C2 | [Data] | 0A
PREFIX      |LEN|IDX|TYP|CT1|CT2| (LEN bytes) |END
  4 bytes   | 1 | 1 | 1 | 1 | 1 |  Variable  | 1
```

## Message Types
| Type | Name | Purpose | Frequency |
|------|------|---------|-----------|
| `0x00` | Keep-Alive | Boot handshake | Boot only |
| `0x01` | Header | Model/firmware info | Once during boot |
| `0x03` | Status | System state | Occasional |
| `0x07` | Sensor | Air/fire readings | 20-30/sec ⭐ |

## Sensor Packet Format (Type 0x07, 8 bytes)
```
Byte 0: Intensity (00-FF)
Bytes 1-2: Big5 Character (sensor type)
Bytes 3-4: Calibration modifier
Bytes 5-7: IEEE 32-bit float
Byte 7: Mystery byte (01-03)
```

## Sensor Type Identification (Big5)
| Bytes | Char | English | Sensor Type | Meaning |
|-------|------|---------|-------------|---------|
| `B9 47` | 逼 | Force | Air | MAX pressure |
| `B8 47` | 矮 | Low | Air | MID pressure |
| `B4 47` | 孱 | Weak | Air | MIN pressure |
| `B3 47` | 蛄 | Mayfly | Air | IDLE (no air) |
| `B7 47` | 廉 | Motion | Air | UP/DOWN |
| `BC 47` | 嘮 | Chat | Air | FLUCTUATING |
| `BA 47` | 慘 | Miserable | Fire | 🔥 **ALERT** |
| `BB 47` | 腐 | Decay | Fire | COOLING |
| `B5 47` | 湟 | Radiance | Fire | PROXIMITY |

## Calibration Value Ranges
| State | Min | Max | Mean | Notes |
|-------|-----|-----|------|-------|
| Max Air (逼) | 3000 | 3100 | ~3050 | Highest pressure setting |
| Mid Air (矮) | 2500 | 3000 | ~2750 | Medium pressure |
| Min Air (孱) | 0 | 1500 | ~750 | Lowest pressure |
| Idle (蛄) | 0 | 500 | ~250 | No air flow |
| Overall | 0 | 3331 | ~1589 | Full range |

## Boot Sequence Timeline
```
0ms:     Laser sends first packet
         ↓
1185ms:  Controller receives → starts boot timer
         ↓
+0ms:    Send Keep-Alive #1
+180ms:  Send Keep-Alive #2
+212ms:  Send Keep-Alive #3 (32ms bump)
+244ms:  Send Keep-Alive #4
+276ms:  Send Control packet #1
         ↓
~2-3sec: Boot complete, normal operation begins
```

## Mystery Byte Distribution
| Value | Freq | Hypothesis |
|-------|------|------------|
| `0x02` | 75% | Standard operation (66% duty) |
| `0x03` | 20% | High intensity (100% duty) |
| `0x01` | 5% | Low intensity (33% duty) |

**Theory**: PWM duty cycle for cooling fan/air pump

## Protocol Implementation Checklist
- [x] Parse 4-byte "ILmp" prefix
- [x] Extract 1-byte LENGTH field
- [x] Increment INDEX counter on TX
- [x] Identify TYPE and extract CTRL bytes
- [x] Extract N-byte data payload
- [x] Verify 0x0A newline terminator
- [x] Echo packets back (for most types)
- [x] Handle large packets (>100 bytes) specially
- [x] Reset on 5+ second idle timeout

## Code Snippets

### Detect Fire Alert
```cpp
if (data[1] == 0xBA && data[2] == 0x47) {
    // Character: 慘 = FIRE DETECTED
    digitalWrite(FIRE_ALARM_PIN, HIGH);
}
```

### Get Air Pressure Level
```cpp
byte air_char = data[2];  // data[1] and [2] are Big5 pair
if (data[1] == 0xB9) {      // 逼 = Max
    airLevel = "MAX";
} else if (data[1] == 0xB8) { // 矮 = Mid
    airLevel = "MID";
} else if (data[1] == 0xB4) { // 孱 = Min
    airLevel = "MIN";
}
```

### Extract Calibration Float
```cpp
// Bytes 5-7 contain 32-bit IEEE float
float calib = *(float*)&data[5];
Serial.print("Calibration: ");
Serial.println(calib);
```

### Decode Mystery Byte
```cpp
byte mystery = data[7];
uint8_t pwm_duty = (mystery / 3) * 100; // 1→33%, 2→66%, 3→100%
analogWrite(FAN_PIN, pwm_duty * 2.55);
```

## Tools
```bash
# Display packet summary
python3 content/ai-review/falcon_parser.py content/falcon-boot-rx.txt

# Export to CSV
python3 content/ai-review/falcon_parser.py --csv content/falcon-boot-rx.txt

# Export to JSON
python3 content/ai-review/falcon_parser.py --json content/falcon-boot-rx.txt
```

## Common Issues
| Symptom | Cause | Fix |
|---------|-------|-----|
| Sensors show yellow | Boot not started | Verify boot delay timing |
| Packets dropped | Index mismatch | Ensure INDEX increments on TX |
| No fire alarm | Packet loss | Check serial connection |
| Protocol error | Wrong big5 byte order | Check bytes [1-2] order |

## Documentation
- **Full spec**: `content/ai-review/PROTOCOL_SPECIFICATION.md` (500+ lines)
- **Analysis**: `content/ai-review/ANALYSIS_SUMMARY.md` (this guide + findings)
- **Parser tool**: `content/ai-review/falcon_parser.py` (Python 3, no deps)

---

**Remember**: The protocol is elegant and simple - just echo packets with an incremented index! 🎯
