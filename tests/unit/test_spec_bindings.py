"""Definition binding checks; no NDF parser or hardware oracle is reimplemented."""

import pytest
from pycircuit._spec import _bindings, check_spec


def test_definition_and_transaction_bindings(tmp_path):
    source = tmp_path / "model.py"
    source.write_text(
        "# ndf: implements=PIPE-STATE-001\n@module\ndef Owner(value):\n"
        "    # ndf: implements=PIPE-COMMIT-001\n"
        "    @rule\n    def commit():\n        return value\n"
    )
    bindings = _bindings(source, tmp_path)
    assert [(b["symbol"], b["targets"]) for b in bindings] == [
        ("Owner", ["PIPE-STATE-001"]),
        ("Owner.commit", ["PIPE-COMMIT-001"]),
    ]
    assert all(b["path"] == "model.py" for b in bindings)


def test_strings_do_not_create_bindings(tmp_path):
    source = tmp_path / "model.py"
    source.write_text('"""# ndf: implements=FAKE-001"""\ndef model():\n    pass\n')
    assert _bindings(source, tmp_path) == []


def test_unattached_annotation_rejected(tmp_path):
    source = tmp_path / "model.py"
    source.write_text("# ndf: implements=STATE-001\nvalue = 1\n")
    with pytest.raises(ValueError):
        _bindings(source, tmp_path)


def test_output_preserves_existing_files(tmp_path):
    output = tmp_path / "evidence"
    output.mkdir()
    saved = output / "spec-check.json"
    saved.write_text("old record\n")
    with pytest.raises(FileExistsError):
        check_spec(str(tmp_path), "unused", str(output))
    assert saved.read_text() == "old record\n"
