"""Verify that the firmware entry point remains small and responsibilities stay split."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src"
LIBRARY = ROOT / "lib"


class SourceLayoutTests(unittest.TestCase):
    def test_firmware_is_split_into_network_and_sensor_modules(self):
        for filename in (
            "NetworkManager.h",
            "SensorReader.h",
            "Config.h",
        ):
            self.assertTrue((LIBRARY / filename).is_file(), f"Missing lib/{filename}")

        for filename in ("NetworkManager.cpp", "SensorReader.cpp"):
            self.assertTrue((SOURCE / filename).is_file(), f"Missing src/{filename}")

    def test_main_only_coordinates_the_firmware_lifecycle(self):
        main_source = (SOURCE / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "NetworkManager.h"', main_source)
        self.assertIn('#include "SensorReader.h"', main_source)
        self.assertNotIn("WiFi.begin(", main_source)
        self.assertNotIn("RS485Serial.write(", main_source)

    def test_source_messages_and_comments_use_ascii_english(self):
        for source_file in SOURCE.glob("*.*pp"):
            source_file.read_text(encoding="utf-8").encode("ascii")

    def test_sensor_reader_uses_the_current_arduinojson_document_api(self):
        sensor_source = (SOURCE / "SensorReader.cpp").read_text(encoding="utf-8")
        self.assertIn("JsonDocument document;", sensor_source)
        self.assertNotIn("DynamicJsonDocument", sensor_source)

    def test_platformio_includes_the_project_header_directory(self):
        platformio_config = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("    -Ilib", platformio_config)

    def test_platformio_uses_clean_project_configuration(self):
        platformio_config = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertNotIn("project_config.h", platformio_config)
        self.assertNotIn("lib_extra_dirs", platformio_config)
        self.assertIn("board_build.partitions = config/partitions/yolo_uno_8MB.csv", platformio_config)
        self.assertTrue((ROOT / "config/partitions/yolo_uno_8MB.csv").is_file())
        self.assertFalse((ROOT / "project_config.h").exists())


if __name__ == "__main__":
    unittest.main()
