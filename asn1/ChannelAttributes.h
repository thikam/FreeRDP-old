/*
 * Generated by asn1c-0.9.22.1409 (http://lionet.info/asn1c)
 * From ASN.1 module "MCS-PROTOCOL-3"
 * 	found in "mcs.asn1"
 * 	`asn1c -fnative-types -fskeletons-copy -fcompound-names -gen-PER`
 */

#ifndef	_ChannelAttributes_H_
#define	_ChannelAttributes_H_


#include <asn_application.h>

/* Including external dependencies */
#include "StaticChannelId.h"
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>
#include <BOOLEAN.h>
#include "UserId.h"
#include "PrivateChannelId.h"
#include <asn_SET_OF.h>
#include <constr_SET_OF.h>
#include "AssignedChannelId.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ChannelAttributes_PR {
	ChannelAttributes_PR_NOTHING,	/* No components present */
	ChannelAttributes_PR_static,
	ChannelAttributes_PR_userId,
	ChannelAttributes_PR_private,
	ChannelAttributes_PR_assigned,
	/* Extensions may appear below */
	
} ChannelAttributes_PR;

/* Forward declarations */
struct NonStandardParameter;

/* ChannelAttributes */
typedef struct ChannelAttributes {
	ChannelAttributes_PR present;
	union ChannelAttributes_u {
		struct ChannelAttributes__Static {
			StaticChannelId_t	 channelId;
			struct ChannelAttributes__Static__nonStandard {
				A_SEQUENCE_OF(struct NonStandardParameter) list;
				
				/* Context for parsing across buffer boundaries */
				asn_struct_ctx_t _asn_ctx;
			} *nonStandard;
			/*
			 * This type is extensible,
			 * possible extensions are below.
			 */
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		} Static;
		struct ChannelAttributes__userId {
			BOOLEAN_t	 joined;
			UserId_t	 userId;
			struct ChannelAttributes__userId__nonStandard {
				A_SEQUENCE_OF(struct NonStandardParameter) list;
				
				/* Context for parsing across buffer boundaries */
				asn_struct_ctx_t _asn_ctx;
			} *nonStandard;
			/*
			 * This type is extensible,
			 * possible extensions are below.
			 */
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		} userId;
		struct ChannelAttributes__private {
			BOOLEAN_t	 joined;
			PrivateChannelId_t	 channelId;
			UserId_t	 manager;
			struct ChannelAttributes__private__admitted {
				A_SET_OF(UserId_t) list;
				
				/* Context for parsing across buffer boundaries */
				asn_struct_ctx_t _asn_ctx;
			} admitted;
			struct ChannelAttributes__private__nonStandard {
				A_SEQUENCE_OF(struct NonStandardParameter) list;
				
				/* Context for parsing across buffer boundaries */
				asn_struct_ctx_t _asn_ctx;
			} *nonStandard;
			/*
			 * This type is extensible,
			 * possible extensions are below.
			 */
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		} private;
		struct ChannelAttributes__assigned {
			AssignedChannelId_t	 channelId;
			struct ChannelAttributes__assigned__nonStandard {
				A_SEQUENCE_OF(struct NonStandardParameter) list;
				
				/* Context for parsing across buffer boundaries */
				asn_struct_ctx_t _asn_ctx;
			} *nonStandard;
			/*
			 * This type is extensible,
			 * possible extensions are below.
			 */
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		} assigned;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ChannelAttributes_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ChannelAttributes;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "NonStandardParameter.h"

#endif	/* _ChannelAttributes_H_ */