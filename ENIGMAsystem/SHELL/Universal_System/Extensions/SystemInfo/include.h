/*

 MIT License
 
 Copyright © 2023-2026 Samuel Venable
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
*/

#include "Universal_System/Extensions/SystemInfo/system.hpp"

namespace enigma_user {

inline std::string os_kernel_name() {
  return ngs::si::os_kernel_name();
}

inline std::string os_device_name() {
  return ngs::si::os_device_name();
}

inline std::string os_kernel_release() {
  return ngs::si::os_kernel_release();
}

inline std::string os_product_name() {
  return ngs::si::os_product_name();
}

inline std::string os_kernel_version() {
  return ngs::si::os_kernel_version();
}

inline std::string os_architecture() {
  return ngs::si::os_architecture();
}

inline std::string os_is_virtual() {
  return ngs::si::os_is_virtual();
}

inline std::string memory_totalram(bool hr) {
  return ngs::si::memory_totalram(hr);
}

inline std::string memory_freeram(bool hr) {
  return ngs::si::memory_freeram(hr); 
}

inline std::string memory_usedram(bool hr) {
  return ngs::si::memory_usedram(hr);
}

inline std::string memory_totalswap(bool hr) {
  return ngs::si::memory_totalswap(hr);
}

inline std::string memory_freeswap(bool hr) {
  return ngs::si::memory_freeswap(hr);
}

inline std::string memory_usedswap(bool hr) {
  return ngs::si::memory_usedswap(hr);
}

inline std::string memory_totalvram(bool hr) {
  return ngs::si::memory_totalvram(hr);
}
  
inline std::string gpu_manufacturer() {
  return ngs::si::gpu_manufacturer();
}

inline std::string gpu_renderer() {
  return ngs::si::gpu_renderer();
}

inline std::string cpu_vendor() {
  return ngs::si::cpu_vendor();
}

inline std::string cpu_processor() {
  return ngs::si::cpu_processor();
}

inline std::string cpu_processor_count() {
  return ngs::si::cpu_processor_count();
}

inline std::string cpu_core_count() {
  return ngs::si::cpu_core_count();
}

} // namespace enigma_user


