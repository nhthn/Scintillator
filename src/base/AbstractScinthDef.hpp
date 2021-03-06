#ifndef SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
#define SRC_CORE_ABSTRACT_SCINTHDEF_HPP_

#include "base/AbstractVGen.hpp"
#include "base/Intrinsic.hpp"
#include "base/Manifest.hpp"
#include "base/Parameter.hpp"
#include "base/RenderOptions.hpp"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <vector>
#include <memory>

namespace scin { namespace base {

class Shape;
class VGen;

/*! Maintains a topologically sorted signal graph of VGens and constructs shaders and requirements for the graphical
 * ScinthDef instances.
 *
 * The AbstractScinthDef takes as input the VGen graph with named parameters, and produces a plain data (meaning no
 * Vulkan-specific data structures) representation of a series of data structures and render commands needed to
 * configure the Vulkan graphics pipeline to draw a Scinth instance of the declared ScinthDef. Specifically it
 * produces as output:
 *
 * a) An optional compute shader, if there are any frame-rate VGens in the graph.
 * b) A unified uniform buffer manifest containing any intrinsic inputs for the ScinthDef as well as compute outputs
 * c) A vertex shader input manifest
 * d) A vertex shader containing both any shape-rate VGens as well as necessary pass-through data to the fragment shader
 * e) A vertex shader output manifest, one of the sources of input to the fragment shader, called the fragment manifest
 * f) Lists of unified Samplers both constant and parameterized for both compute and draw stages
 * g) Lists of parameters provided as push constants for both compute and draw stages
 * h) A fragment shader with the pixel-rate VGens
 *
 * Overall approach is to take a first pass through the graph from output back to inputs, validating the rate flow and
 * bucketing the VGens into one of compute, vertex, or fragment shader groups. Then we take a forward pass through each
 * group of VGens to generate manifests and shaders.
 *
 */
class AbstractScinthDef {
public:
    /*! Copy the supplied list of VGens into self and construct an AbstractScinthDef.
     *
     * \param name The name of this ScinthDef.
     * \param shape The shape object associated with this.
     * \param renderOptions A RenderOptions object detailing the options to use rendering this ScinthDef
     * \param parameters A list of parameter objects, in order.
     * \param instances The VGens in topological order from inputs to output. It is assumed these have been individually
     *        validated already, meaning that each VGen has appropriate rate and input configuration.
     */
    AbstractScinthDef(const std::string& name, std::unique_ptr<Shape> shape, const RenderOptions& renderOptions,
                      const std::vector<Parameter>& parameters, const std::vector<VGen>& instances);

    /*! Destructs an AbstractScinthDef and all associated resources.
     */
    ~AbstractScinthDef();

    /*! Construct shaders, uniform list, and other requirements for rendering this ScinthDef.
     *
     * \return true if successful, false on error.
     */
    bool build();

    /*! Returns the index for a given parameter name, or -1 if name not found.
     *
     * \param name The name of the parameter to look up.
     * \param indexOut The output index if found.
     * \return True if index found, false if not.
     */
    bool indexForParameterName(const std::string& name, size_t& indexOut) const;

    const std::string& name() const { return m_name; }
    const Shape* shape() const { return m_shape.get(); }
    const RenderOptions& renderOptions() const { return m_renderOptions; }
    const std::vector<Parameter>& parameters() const { return m_parameters; }
    const std::vector<VGen>& instances() const { return m_instances; }

    // First element in pair is sampler key, second element is the imageID.
    const std::set<std::pair<uint32_t, size_t>>& computeFixedImages() const { return m_computeFixedImages; }
    const std::set<std::pair<uint32_t, size_t>>& drawFixedImages() const { return m_drawFixedImages; }
    // First element in pair is sampler key, second is parameter index.
    const std::set<std::pair<uint32_t, size_t>>& computeParameterizedImages() const {
        return m_computeParameterizedImages;
    }
    const std::set<std::pair<uint32_t, size_t>>& drawParameterizedImages() const { return m_drawParameterizedImages; }

    bool hasComputeStage() const { return m_hasComputeStage; }

    const std::string& prefix() const { return m_prefix; }
    const std::string& computeShader() const { return m_computeShader; }
    const std::string& vertexShader() const { return m_vertexShader; }
    const std::string& fragmentShader() const { return m_fragmentShader; }
    const Manifest& fragmentManifest() const { return m_fragmentManifest; }
    const Manifest& vertexManifest() const { return m_vertexManifest; }
    const Manifest& uniformManifest() const { return m_uniformManifest; }
    const Manifest& computeManifest() const { return m_computeManifest; }

private:
    /*! Recursively traverses the VGens list from output back to inputs, grouping them into compute, vertex, and
     *  fragment shaders. Also checks the progression of rates as valid. Groups the Sampling VGen image inputs into
     *  sampler/image pairs for compute and draw shaders.
     *
     * \param index The index in m_instances of the node to check in this recursive call. Starts at the end of the
     *        vector and descends to zero.
     * \param maxRate The maximum rate to accept, to validate against rate decreases only from output to input.
     * \param computeVGens A reference in which to store the indices of any frame-rate VGens in the graph.
     * \param vertexVGens A reference in which to store the indices of any shape-rate VGens in the graph.
     * \param fragmentVGens A reference in which to store the indices of any pixel-rate VGens in the
     *        graph.
     * \return true if successful, false on error.
     */
    bool groupVGens(size_t index, AbstractVGen::Rates rate, std::set<size_t>& computeVGens,
                    std::set<size_t>& vertexVGens, std::set<size_t>& fragmentVGens);

    bool buildComputeStage(const std::set<size_t>& computeVGens);
    bool buildDrawStage(const std::set<size_t>& vertexVGens, const std::set<size_t>& fragmentVGens);
    bool finalizeShaders();

    std::string m_name;
    std::unique_ptr<Shape> m_shape;
    RenderOptions m_renderOptions;
    std::vector<Parameter> m_parameters;
    std::vector<VGen> m_instances;

    // These are pairs of sampler config, image or parameter index, grouped into sets to de-dupe the pairs.
    std::set<std::pair<uint32_t, size_t>> m_computeFixedImages;
    std::set<std::pair<uint32_t, size_t>> m_computeParameterizedImages;
    std::set<std::pair<uint32_t, size_t>> m_drawFixedImages;
    std::set<std::pair<uint32_t, size_t>> m_drawParameterizedImages;

    // To avoid collision with any VGen code we attach a ScinthDef name and random number prefix to most global names.
    std::string m_prefix;
    // Hard-coded single output from fragment shader is color.
    std::string m_fragmentOutputName;

    // Outputs from the vertex shader that are supplied as inputs to the fragment shader.
    Manifest m_fragmentManifest;

    // Inputs to, and outputs from compute shader, and any intrinsics are supplied as a uniform to all shaders.
    Manifest m_uniformManifest;

    // Intrinsic and Shape inputs to the vertex shader.
    Manifest m_vertexManifest;

    // Outputs from compute shader for reading in draw stages.
    Manifest m_computeManifest;

    std::string m_fragmentShader;
    std::string m_vertexShader;
    std::string m_computeShader;

    bool m_hasComputeStage;

    std::unordered_map<std::string, size_t> m_parameterIndices;
};

} // namespace base
} // namespace scin

#endif // SRC_CORE_ABSTRACT_SCINTHDEF_HPP_
