/*

					Label data internals






!Label data internals!

*/

/*
**	(c) COPYRIGHT MIT 1996.
**	Please first read the full copyright statement in the file COPYRIGH.
*/

/*

This module defines the Label data structures read by 
CSParser.c. Applications will include this if they
want direct access to the data (as opposed to using iterator methods).

The following data structures relate to the data encapsulated in a PICS Label.
Each data type correlates to a time in the BNF for the label description.
See PICS Labels spec
for more details.

*/

#ifndef CSLLST_H
#define CSLLST

/*

.Label Error.
combination of:
o label-error
o service-error
o service-info 'no-ratings'

*/

typedef struct {
    LabelErrorCode_t errorCode;
    HTList * explanations; /* HTList of (char *) */
    } LabelError_t;

/*

.Extension Data.
called data in the BNF

*/

typedef struct ExtensionData_s ExtensionData_t;
struct ExtensionData_s {
    char * text;
    BOOL quoted;
    HTList * moreData;
    ExtensionData_t * pParentExtensionData;
    };

/*

.Extension.
option 'extension'

*/

typedef struct {
    BOOL mandatory;
    SVal_t url;
    HTList * extensionData;
    } Extension_t;

/*

.Label Options.
called option in the BNF

*/

typedef struct LabelOptions_s LabelOptions_t;
struct LabelOptions_s {
    DVal_t at;
    SVal_t by;
    SVal_t complete_label;
    BVal_t generic;
    SVal_t fur; /* for is a reserved word */
    SVal_t MIC_md5;
    DVal_t on;
    SVal_t signature_PKCS;
    DVal_t until;
    HTList * comments;
    HTList * extensions;
    /* find service-level label options */
    LabelOptions_t * pParentLabelOptions;
    };


/*

.Rating.
called rating in the BNF

*/

typedef struct {
    SVal_t identifier;
    FVal_t value;
    HTList * ranges;
    } LabelRating_t;

/*

.SingleLabel.
called single-label in the BNF

*/

typedef struct {
    LabelOptions_t * pLabelOptions;
    HTList * labelRatings;
    } SingleLabel_t;


/*

.Label.
also called label

*/

typedef struct {
    LabelError_t * pLabelError;
    HTList * singleLabels;
    SingleLabel_t * pSingleLabel;
    } Label_t;

/*

.ServiceInfo.
called service-info in the BNF

*/

typedef struct {
    SVal_t rating_service;
    LabelOptions_t * pLabelOptions;
    LabelError_t * pLabelError;
    HTList * labels;
    } ServiceInfo_t;

/*

.CSLLData.
The whole shebang.

*/

struct CSLLData_s {
    FVal_t version;
    LabelError_t * pLabelError;
    HTList * serviceInfos;

    /* some usefull flags */
    BOOL complete;
    BOOL hasTree; /* so it can't make a list of labels */
    int mandatoryExtensions;
    };

/*


--------------these need the above structures--------------

*/

extern CSLLData_t * CSLabel_getCSLLData(CSLabel_t * me);
extern LabelError_t * CSLabel_getLabelError(CSLabel_t * pCSLabel);
extern LabelOptions_t * CSLabel_getLabelOptions(CSLabel_t * pCSLabel);
extern ServiceInfo_t * CSLabel_getServiceInfo(CSLabel_t * pCSLabel);
extern Label_t * CSLabel_getLabel(CSLabel_t * pCSLabel);
extern SingleLabel_t * CSLabel_getSingleLabel(CSLabel_t * pCSLabel);
extern LabelRating_t * CSLabel_getLabelRating(CSLabel_t * pCSLabel);

/*

*/

#endif /* CSLLST_H */

/*

End of Declaration

*/
