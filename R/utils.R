env_to_logical <- function (var, default = TRUE) {
  res <- Sys.getenv(var, default)
  if (res %in% c("1", "yes", "true")) {
    TRUE
  } else if (res %in% c("0", "no", "false")) {
    FALSE
  } else {
    default
  }
}

is_windows <- function() {
  identical(tolower(Sys.info()[["sysname"]]), "windows")
}
