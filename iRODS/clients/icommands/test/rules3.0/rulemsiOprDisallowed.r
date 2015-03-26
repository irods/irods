acSetRescSchemeForCreate {
  ON ($objPath like "\*foo*") {
    msiOprDisallowed;
  } 
}