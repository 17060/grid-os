"""Shared Flynn GridFS (GFS2FLYN) layout for host tools."""

import struct
from pathlib import Path

GFS_MAGIC = b"GFS2FLYN"
GFS_VERSION = 3
GFS_SUPER_LBA = 64
GFS_INODE_TABLE_LBA = 65
GFS_INODE_TABLE_SECTORS = 128
GFS_INODE_MAX = 1024
GFS_SECTORS_PER_FILE = 128
GFS_DATA_BASE_LBA = 128
GFS_INODE_MAGIC = 0x46494C45
SECTOR = 512
GFS_FILE_CAP = GFS_SECTORS_PER_FILE * SECTOR


def crc32(data: bytes) -> int:
    import binascii

    return binascii.crc32(data) & 0xFFFFFFFF


def build_superblock() -> bytes:
    sb = bytearray(SECTOR)
    sb[0:8] = GFS_MAGIC
    struct.pack_into("<I", sb, 8, GFS_VERSION)
    struct.pack_into("<I", sb, 12, GFS_INODE_MAX)
    struct.pack_into("<I", sb, 16, GFS_DATA_BASE_LBA)
    struct.pack_into("<I", sb, 20, GFS_SECTORS_PER_FILE)
    checksum = crc32(bytes(sb[:24]))
    struct.pack_into("<I", sb, 24, checksum)
    return bytes(sb)


def encode_inode(path: str, size: int) -> bytes:
    path_bytes = path.encode("ascii")[:55]
    entry = struct.pack("<II", GFS_INODE_MAGIC, size)
    entry += path_bytes + b"\x00" * (56 - len(path_bytes))
    return entry


def write_sector(disk: bytearray, lba: int, data: bytes) -> None:
    offset = lba * SECTOR
    disk[offset : offset + SECTOR] = data[:SECTOR].ljust(SECTOR, b"\x00")


def find_inode_slot(disk: bytearray, path: str) -> int | None:
    for slot in range(GFS_INODE_MAX):
        inode_off = slot * 64
        table_sector = GFS_INODE_TABLE_LBA + (inode_off // SECTOR)
        table_offset = inode_off % SECTOR
        sector_start = table_sector * SECTOR
        sector = disk[sector_start : sector_start + SECTOR]
        magic, size = struct.unpack_from("<II", sector, table_offset)
        if magic != GFS_INODE_MAGIC:
            continue
        stored = sector[table_offset + 8 : table_offset + 64].split(b"\x00", 1)[0].decode("ascii")
        if stored == path:
            return slot
    return None


def find_free_slot(disk: bytearray) -> int | None:
    for slot in range(1, GFS_INODE_MAX):
        inode_off = slot * 64
        table_sector = GFS_INODE_TABLE_LBA + (inode_off // SECTOR)
        table_offset = inode_off % SECTOR
        sector_start = table_sector * SECTOR
        sector = disk[sector_start : sector_start + SECTOR]
        magic, _ = struct.unpack_from("<II", sector, table_offset)
        if magic != GFS_INODE_MAGIC:
            return slot
    return None


def seed_file(disk: bytearray, slot: int, path: str, payload: bytes) -> None:
    if len(payload) > GFS_FILE_CAP:
        raise SystemExit(f"{path}: file too large ({len(payload)} bytes, max {GFS_FILE_CAP})")

    inode_off = slot * 64
    table_sector = GFS_INODE_TABLE_LBA + (inode_off // SECTOR)
    table_offset = inode_off % SECTOR

    inode = encode_inode(path, len(payload))
    sector_start = table_sector * SECTOR
    sector = bytearray(disk[sector_start : sector_start + SECTOR])
    sector[table_offset : table_offset + 64] = inode
    write_sector(disk, table_sector, bytes(sector))

    base_lba = GFS_DATA_BASE_LBA + slot * GFS_SECTORS_PER_FILE
    padded = payload.ljust(GFS_FILE_CAP, b"\x00")
    for i in range(GFS_SECTORS_PER_FILE):
        write_sector(disk, base_lba + i, padded[i * SECTOR : (i + 1) * SECTOR])


def install_file(disk_path: Path, vfs_path: str, payload: bytes) -> None:
    if not vfs_path.startswith("/"):
        raise SystemExit(f"path must be absolute: {vfs_path}")

    disk = bytearray(disk_path.read_bytes())
    min_sectors = GFS_DATA_BASE_LBA + GFS_INODE_MAX * GFS_SECTORS_PER_FILE
    if len(disk) < min_sectors * SECTOR:
        raise SystemExit("Disk too small for GFS2FLYN layout")

    slot = find_inode_slot(disk, vfs_path)
    if slot is None:
        slot = find_free_slot(disk)
    if slot is None:
        raise SystemExit("No free GFS inode slots")

    seed_file(disk, slot, vfs_path, payload)
    disk_path.write_bytes(disk)
