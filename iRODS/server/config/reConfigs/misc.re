acExtractMetadata {ON($dataType == "fits image") {msiExtractMetadataForFitsImage ::: recover_msiExtractMetadataForFitsImage;}}
acExtractMetadata {ON($dataType == "dicom image") {msiExtractMetadataForDicomData ::: recover_msiExtractMetadataForDicomData;}}
