#!/bin/bash

# Simple performance and consistency test.
#

set +e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/detect_vars.sh

echo "PG evaluation test for ${HCVERSION} from ${HCDIR}"

HG19=${DIR}/../../example/chr21.fa

TMP_OUT=`mktemp -t happy.XXXXXXXXXX`

# run hap.py
${PYTHON} ${HCDIR}/hap.py \
			 	-l chr21 \
			 	${DIR}/../../example/happy/PG_NA12878_chr21.vcf.gz \
			 	${DIR}/../../example/happy/NA12878_chr21.vcf.gz \
			 	-f ${DIR}/../../example/happy/PG_Conf_chr21.bed.gz \
			 	-r ${DIR}/../../example/chr21.fa \
			 	-o ${TMP_OUT} \
			 	-X -P \
			 	--force-interactive

if [[ $? != 0 ]]; then
	echo "hap.py failed!"
	exit 1
fi

# This script checks if the summary precision / recall figures have changed significantly
${PYTHON} ${DIR}/compare_summaries.py ${TMP_OUT}.summary.csv ${DIR}/../../example/happy/expected.summary.csv
if [[ $? != 0 ]]; then
	echo "All summary differs! -- diff ${TMP_OUT}.summary.csv ${DIR}/../../example/happy/expected.summary.csv"
	exit 1
fi

# run hap.py
${PYTHON} ${HCDIR}/hap.py \
			 	-l chr21 \
			 	${DIR}/../../example/happy/PG_NA12878_chr21.vcf.gz \
			 	${DIR}/../../example/happy/NA12878_chr21.vcf.gz \
			 	-f ${DIR}/../../example/happy/PG_Conf_chr21.bed.gz \
			 	-r ${DIR}/../../example/chr21.fa \
			 	-o ${TMP_OUT}.pass \
			 	-X \
			 	--force-interactive

if [[ $? != 0 ]]; then
	echo "hap.py failed!"
	exit 1
fi

${PYTHON} ${DIR}/compare_summaries.py ${TMP_OUT}.pass.summary.csv ${DIR}/../../example/happy/expected.pass.summary.csv
if [[ $? != 0 ]]; then
	echo "PASS summary differs! -- diff ${TMP_OUT}.pass.summary.csv ${DIR}/../../example/happy/expected.pass.summary.csv"
	exit 1
fi

# run hap.py
${PYTHON} ${HCDIR}/hap.py \
			 	-l chr21 \
			 	${DIR}/../../example/happy/PG_NA12878_chr21.vcf.gz \
			 	${DIR}/../../example/happy/NA12878_chr21.vcf.gz \
			 	-f ${DIR}/../../example/happy/PG_Conf_chr21.bed.gz \
			 	-r ${DIR}/../../example/chr21.fa \
			 	-o ${TMP_OUT}.unhappy \
			 	-X --unhappy \
			 	--roc VQSLOD \
			 	-P \
			 	--roc-filter TruthSensitivityTranche99.00to99.90,TruthSensitivityTranche99.90to100.00,TruthSensitivityTranche99.90to100.00+ \
			 	--force-interactive

if [[ $? != 0 ]]; then
	echo "hap.py failed!"
	exit 1
fi

${PYTHON} ${DIR}/compare_summaries.py ${TMP_OUT}.unhappy.summary.csv ${DIR}/../../example/happy/expected.unhappy.summary.csv
if [[ $? != 0 ]]; then
	echo "PASS summary differs! -- diff ${TMP_OUT}.unhappy.summary.csv ${DIR}/../../example/happy/expected.unhappy.summary.csv"
	exit 1
fi

rm -f ${TMP_OUT}*
echo "PG quantification test SUCCEEDED!"

