export module math:settings;

namespace math {

/** Possible implementations for a math function */
export enum class Implementation {
  Default,    //!< Use the C++ standard library implementation
  CmsisDsp,   //!< Use the CMSIS-DSP implementation
};

/** Settings for math functions */
export struct Settings {
  Implementation implementation =
      Implementation::Default;   //!< Implementation to use to evaluate a
                                 //!< function
};

}   // namespace math