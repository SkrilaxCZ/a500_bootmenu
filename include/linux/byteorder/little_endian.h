#ifndef _LINUX_BYTEORDER_LITTLE_ENDIAN_H
#define _LINUX_BYTEORDER_LITTLE_ENDIAN_H

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __LITTLE_ENDIAN_BITFIELD
#define __LITTLE_ENDIAN_BITFIELD
#endif
#define	__BYTE_ORDER	__LITTLE_ENDIAN

#include <linux/byteorder/swab.h>

#define __constant_htonl(x) ___constant_swab32((x))
#define __constant_ntohl(x) ___constant_swab32((x))
#define __constant_htons(x) ___constant_swab16((x))
#define __constant_ntohs(x) ___constant_swab16((x))
#define __constant_cpu_to_le64(x) ((__u64)(x))
#define __constant_le64_to_cpu(x) ((__u64)(x))
#define __constant_cpu_to_le32(x) ((__u32)(x))
#define __constant_le32_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_le16(x) ((__u16)(x))
#define __constant_le16_to_cpu(x) ((__u16)(x))
#define __constant_cpu_to_be64(x) ___constant_swab64((x))
#define __constant_be64_to_cpu(x) ___constant_swab64((x))
#define __constant_cpu_to_be32(x) ___constant_swab32((x))
#define __constant_be32_to_cpu(x) ___constant_swab32((x))
#define __constant_cpu_to_be16(x) ___constant_swab16((x))
#define __constant_be16_to_cpu(x) ___constant_swab16((x))
#define __cpu_to_le64(x) ((__u64)(x))
#define __le64_to_cpu(x) ((__u64)(x))
#define __cpu_to_le32(x) ((__u32)(x))
#define __le32_to_cpu(x) ((__u32)(x))
#define __cpu_to_le16(x) ((__u16)(x))
#define __le16_to_cpu(x) ((__u16)(x))
#define __cpu_to_be64(x) __swab64((x))
#define __be64_to_cpu(x) __swab64((x))
#define __cpu_to_be32(x) __swab32((x))
#define __be32_to_cpu(x) __swab32((x))
#define __cpu_to_be16(x) __swab16((x))
#define __be16_to_cpu(x) __swab16((x))
#define __cpu_to_le64p(x) (*(__u64*)(x))
#define __le64_to_cpup(x) (*(__u64*)(x))
#define __cpu_to_le32p(x) (*(__u32*)(x))
#define __le32_to_cpup(x) (*(__u32*)(x))
#define __cpu_to_le16p(x) (*(__u16*)(x))
#define __le16_to_cpup(x) (*(__u16*)(x))
#define __cpu_to_be64p(x) __swab64p((x))
#define __be64_to_cpup(x) __swab64p((x))
#define __cpu_to_be32p(x) __swab32p((x))
#define __be32_to_cpup(x) __swab32p((x))
#define __cpu_to_be16p(x) __swab16p((x))
#define __be16_to_cpup(x) __swab16p((x))
#define __cpu_to_le64s(x) do {} while (0)
#define __le64_to_cpus(x) do {} while (0)
#define __cpu_to_le32s(x) do {} while (0)
#define __le32_to_cpus(x) do {} while (0)
#define __cpu_to_le16s(x) do {} while (0)
#define __le16_to_cpus(x) do {} while (0)
#define __cpu_to_be64s(x) __swab64s((x))
#define __be64_to_cpus(x) __swab64s((x))
#define __cpu_to_be32s(x) __swab32s((x))
#define __be32_to_cpus(x) __swab32s((x))
#define __cpu_to_be16s(x) __swab16s((x))
#define __be16_to_cpus(x) __swab16s((x))

#define cpu_to_le64 __cpu_to_le64
#define le64_to_cpu __le64_to_cpu
#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu
#define cpu_to_be64 __cpu_to_be64
#define be64_to_cpu __be64_to_cpu
#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu
#define cpu_to_be16 __cpu_to_be16
#define be16_to_cpu __be16_to_cpu
#define cpu_to_le64p __cpu_to_le64p
#define le64_to_cpup __le64_to_cpup
#define cpu_to_le32p __cpu_to_le32p
#define le32_to_cpup __le32_to_cpup
#define cpu_to_le16p __cpu_to_le16p
#define le16_to_cpup __le16_to_cpup
#define cpu_to_be64p __cpu_to_be64p
#define be64_to_cpup __be64_to_cpup
#define cpu_to_be32p __cpu_to_be32p
#define be32_to_cpup __be32_to_cpup
#define cpu_to_be16p __cpu_to_be16p
#define be16_to_cpup __be16_to_cpup
#define cpu_to_le64s __cpu_to_le64s
#define le64_to_cpus __le64_to_cpus
#define cpu_to_le32s __cpu_to_le32s
#define le32_to_cpus __le32_to_cpus
#define cpu_to_le16s __cpu_to_le16s
#define le16_to_cpus __le16_to_cpus
#define cpu_to_be64s __cpu_to_be64s
#define be64_to_cpus __be64_to_cpus
#define cpu_to_be32s __cpu_to_be32s
#define be32_to_cpus __be32_to_cpus
#define cpu_to_be16s __cpu_to_be16s
#define be16_to_cpus __be16_to_cpus

#endif /* _LINUX_BYTEORDER_LITTLE_ENDIAN_H */
