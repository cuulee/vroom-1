% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/vroom.R
\name{vroom_progress}
\alias{vroom_progress}
\title{Determine progress bars should be shown}
\usage{
vroom_progress()
}
\description{
Progress bars are shown _unless_ one of the following is `TRUE`
- The bar is explicitly disabled by setting `Sys.getenv("VROOM_SHOW_PROGRESS"="false")`
- The code is run in a non-interactive session (`interactive()` is `FALSE`).
- The code is run in an RStudio notebook chunk.
- The code is run by knitr / rmarkdown.
- The code is run by testthat (the `TESTTHAT` envvar is `true`).
}
