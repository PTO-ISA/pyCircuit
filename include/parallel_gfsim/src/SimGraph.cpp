#include "parallel_gfsim/SimSystem.h"

#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "parallel_gfsim/SimModule.h"

namespace parallel_gfsim {

namespace {

std::string DotQuote(const std::string& value)
{
    std::string escaped = "\"";
    for (char ch : value) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        default: escaped += ch; break;
        }
    }
    return escaped + '"';
}

struct QueueNode {
    const SimObj* object;
    const SimModule* owner;
    std::size_t id;
};

struct ModuleNode {
    const SimModule* module;
    std::size_t id;
};

} // namespace

void SimSystem::WriteDot(std::ostream& output) const
{
    if (!built_) {
        throw std::logic_error("call Build before WriteDot");
    }

    std::vector<QueueNode> queues;
    std::vector<ModuleNode> modules;
    std::vector<const SimObj*> otherRoots;
    std::map<const SimObj*, std::size_t> queueIds;
    std::map<const SimModule*, std::size_t> moduleIds;

    const auto addQueue = [&](const SimObj* queue, const SimModule* owner) {
        const auto found = queueIds.find(queue);
        if (found != queueIds.end()) {
            if (owner != nullptr) {
                queues[found->second].owner = owner;
            }
            return;
        }
        const std::size_t id = queues.size();
        queueIds.emplace(queue, id);
        queues.push_back(QueueNode{queue, owner, id});
    };

    for (const auto& root : modules_) {
        if (dynamic_cast<const SimModule*>(root.get()) == nullptr) {
            otherRoots.push_back(root.get());
        }
    }

    std::function<void(const SimModule*)> collect;
    collect = [&](const SimModule* module) {
        if (moduleIds.count(module) != 0) {
            return;
        }
        const std::size_t id = modules.size();
        moduleIds.emplace(module, id);
        modules.push_back(ModuleNode{module, id});
        for (const auto& queue : module->innerQueues_) {
            addQueue(queue.get(), module);
        }
        for (const auto& child : module->submodules_) {
            collect(child.get());
        }
        for (const SimObj* queue : module->inputs_) {
            addQueue(queue, nullptr);
        }
        for (const SimObj* queue : module->outputs_) {
            addQueue(queue, nullptr);
        }
    };
    for (const auto& root : modules_) {
        if (const auto* module = dynamic_cast<const SimModule*>(root.get())) {
            collect(module);
        }
    }
    for (const auto& queue : queues) {
        if (queue.owner == nullptr) {
            throw std::logic_error("DOT queue has no owning parent module");
        }
    }

    output << "digraph GFSIM {\n"
           << "  graph [compound=true, rankdir=LR, fontname=Helvetica];\n"
           << "  node [fontname=Helvetica];\n"
           << "  edge [fontname=Helvetica];\n";
    const auto emitQueue = [&](const QueueNode& queue, const std::string& indent) {
        const std::string path = queue.owner == nullptr
            ? queue.object->Name()
            : queue.owner->Path() + "/" + queue.object->Name();
        output << indent << 'q' << queue.id
               << " [shape=cylinder, style=filled, fillcolor=\"#e6f2ff\", "
               << "label=" << DotQuote("Queue\n" + queue.object->Name())
               << ", tooltip=" << DotQuote(path) << "];\n";
    };
    for (std::size_t index = 0; index < otherRoots.size(); ++index) {
        output << "  o" << index << " [shape=box, label="
               << DotQuote(otherRoots[index]->Name()) << "];\n";
    }

    std::function<void(const SimModule*, const std::string&)> emitModule;
    emitModule = [&](const SimModule* module, const std::string& indent) {
        const std::size_t id = moduleIds.at(module);
        output << indent << "subgraph cluster_m" << id << " {\n"
               << indent << "  label=" << DotQuote(module->Name()) << ";\n"
               << indent << "  color=\"#8a9aab\"; style=rounded;\n"
               << indent << "  m" << id << " [shape=box, style=filled, "
               << "fillcolor=\"#f5f5f5\", label="
               << DotQuote(module->Name()) << "];\n";
        for (const auto& queue : queues) {
            if (queue.owner == module) {
                emitQueue(queue, indent + "  ");
            }
        }
        for (const auto& child : module->submodules_) {
            emitModule(child.get(), indent + "  ");
        }
        output << indent << "}\n";
    };
    for (const auto& root : modules_) {
        if (const auto* module = dynamic_cast<const SimModule*>(root.get())) {
            emitModule(module, "  ");
        }
    }
    for (const auto& entry : modules) {
        const SimModule* module = entry.module;
        for (const SimObj* queue : module->inputs_) {
            output << "  q" << queueIds.at(queue) << " -> m" << entry.id
                   << " [label=\"input\"];\n";
        }
        for (const SimObj* queue : module->outputs_) {
            output << "  m" << entry.id << " -> q" << queueIds.at(queue)
                   << " [label=\"output\"];\n";
        }
        for (const SimModule* target : module->moduleTargets_) {
            output << "  m" << entry.id << " -> m" << moduleIds.at(target)
                   << " [style=dashed, label=\"method\"];\n";
        }
    }
    output << "}\n";
    if (!output) {
        throw std::runtime_error("failed to write DOT graph");
    }
}

void SimSystem::SaveDot(const std::string& path) const
{
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("cannot open DOT output: " + path);
    }
    WriteDot(output);
    output.close();
    if (!output) {
        throw std::runtime_error("failed to close DOT output: " + path);
    }
}

} // namespace parallel_gfsim
