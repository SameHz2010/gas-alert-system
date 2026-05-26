import re
from pathlib import Path

import firebase_admin
import joblib
import pandas as pd
from firebase_admin import credentials, db

BASE_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = BASE_DIR.parent
SERVICE_ACCOUNT_PATH = BASE_DIR / "secrets" / "serviceAccountKey.json"
MODEL_PATH = BASE_DIR / "models" / "gas_model.pkl"
SECRETS_HEADER_PATH = PROJECT_ROOT / "include" / "secrets.h"
CONFIG_HEADER_PATH = PROJECT_ROOT / "include" / "config.h"
DATABASE_URL = "https://sensor-gas-3843e-default-rtdb.firebaseio.com/"
DEVICES_PATH = "devices"
FEATURE_COLUMNS = [
    "temperature",
    "humidity",
    "gas",
    "delta_gas",
    "gas_relative",
]


def init_firebase(reset=False):
    if not SERVICE_ACCOUNT_PATH.exists():
        raise FileNotFoundError(f"Missing Firebase key: {SERVICE_ACCOUNT_PATH}")

    if reset and firebase_admin._apps:
        firebase_admin.delete_app(firebase_admin.get_app())

    if not firebase_admin._apps:
        cred = credentials.Certificate(str(SERVICE_ACCOUNT_PATH))
        firebase_admin.initialize_app(
            cred,
            {
                "databaseURL": DATABASE_URL,
            },
        )

    return db.reference("/")


def get_db_reference(path=""):
    root_ref = init_firebase()
    path = (path or "").strip("/")
    return root_ref if not path else root_ref.child(path)


def load_device_id(header_path=CONFIG_HEADER_PATH):
    if not Path(header_path).exists():
        raise FileNotFoundError(f"Missing config header: {header_path}")

    content = Path(header_path).read_text(encoding="utf-8")
    match = re.search(r'#define\s+DEVICE_ID\s+"([^"]+)"', content)
    if not match:
        raise ValueError(f"Cannot find DEVICE_ID in {header_path}")
    return match.group(1)


def fetch_dataset_frame(device_id=None):
    root_ref = get_db_reference(DEVICES_PATH)
    devices_data = root_ref.get() or {}

    rows = []

    if isinstance(devices_data, dict):
        for current_device_id, payload in devices_data.items():
            if device_id and current_device_id != device_id:
                continue

            if not isinstance(payload, dict):
                continue

            dataset = payload.get("dataset", {})
            if not isinstance(dataset, dict):
                continue

            for sample in dataset.values():
                if not isinstance(sample, dict):
                    continue

                sample_row = dict(sample)
                sample_row.setdefault("device_id", current_device_id)
                rows.append(sample_row)

    if rows:
        return pd.DataFrame(rows)

    # Backward compatibility for old dataset structure
    legacy_ref = get_db_reference("dataset")
    legacy_data = legacy_ref.get() or {}
    if not isinstance(legacy_data, dict):
        return pd.DataFrame()

    legacy_rows = []
    for sample in legacy_data.values():
        if not isinstance(sample, dict):
            continue
        sample_row = dict(sample)
        sample_row.setdefault("device_id", "legacy")
        legacy_rows.append(sample_row)

    return pd.DataFrame(legacy_rows)


def load_model():
    if not MODEL_PATH.exists():
        raise FileNotFoundError(f"Missing model file: {MODEL_PATH}")

    return joblib.load(MODEL_PATH)


def validate_feature_columns(df):
    missing = [column for column in FEATURE_COLUMNS if column not in df.columns]
    if missing:
        raise ValueError(f"Missing feature columns: {missing}")


def build_feature_frame(payload):
    frame = pd.DataFrame([{column: payload[column] for column in FEATURE_COLUMNS}])
    validate_feature_columns(frame)
    return frame


def load_twilio_config(header_path=SECRETS_HEADER_PATH):
    if not Path(header_path).exists():
        raise FileNotFoundError(f"Missing Twilio header: {header_path}")

    content = Path(header_path).read_text(encoding="utf-8")
    keys = (
        "TWILIO_ACCOUNT_SID",
        "TWILIO_AUTH_TOKEN",
        "TWILIO_FROM_NUMBER",
        "TWILIO_TO_NUMBER",
    )

    config = {}
    for key in keys:
        pattern = rf'#define\s+{key}\s+"([^"]+)"'
        match = re.search(pattern, content)
        if not match:
            raise ValueError(f"Cannot find {key} in {header_path}")
        config[key] = match.group(1)

    return config
