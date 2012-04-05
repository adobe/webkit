description("Test scientific numbers on <length> values for SVG presentation attributes.")
createSVGTestCase();

var text = createSVGElement("text");
text.setAttribute("id", "text");
text.setAttribute("x", "100px");
text.setAttribute("y", "100px");
rootSVGElement.appendChild(text);


// Testing <length> values for 'font-size'
debug("Test default font-size of '16px'");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");

text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test positive exponent values with 'e'");
text.setAttribute("font-size", ".5e2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "5e1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "0.5e2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+.5e2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5e1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+0.5e2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", ".5e+2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "5e+1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "0.5e+2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test positive exponent values with 'E'");
text.setAttribute("font-size", ".5E2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "5E1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "0.5E2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+.5E2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5E1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+0.5E2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", ".5E+2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "5E+1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "0.5E+2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test negative exponent values with 'e'");
text.setAttribute("font-size", "5000e-2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "500e-1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5000e-2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+500e-1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5000e-2px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+500e-1px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test negative exponent values with 'E'");
text.setAttribute("font-size", "5000E-2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "500E-1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5000E-2");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+500E-1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+5000.00E-2px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "+500E-1px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test negative numbers with exponents");
text.setAttribute("baseline-shift", "-.5e2px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "-50px");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");

text.setAttribute("baseline-shift", "-0.5e2px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "-50px");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");

text.setAttribute("baseline-shift", "-500e-1px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "-50px");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");


debug("");
debug("Test if value and 'em' still works");
text.setAttribute("baseline-shift", "50em");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "50em");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");


debug("");
debug("Trailing and leading whitespaces");
text.setAttribute("font-size", "       5e1");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "5e1      ");
shouldBeEqualToString("getComputedStyle(text).fontSize", "50px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


debug("");
debug("Test behavior on overflow");
text.setAttribute("baseline-shift", "2E+500");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "baseline");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");

text.setAttribute("baseline-shift", "-2E+500");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "baseline");
// Reset baseline-shift to 100px.
text.setAttribute("baseline-shift", "100px");
shouldBeEqualToString("getComputedStyle(text).baselineShift", "100px");


debug("");
debug("Invalid values");
text.setAttribute("font-size", "50e0.0");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "50 e0");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "50e 0");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "50e0 px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");

text.setAttribute("font-size", "50.e0");
shouldBeEqualToString("getComputedStyle(text).fontSize", "16px");
// Reset font-size to 100px.
text.setAttribute("font-size", "100px");
shouldBeEqualToString("getComputedStyle(text).fontSize", "100px");


var successfullyParsed = true;

completeTest();
