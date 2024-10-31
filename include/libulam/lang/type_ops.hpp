#pragma once

namespace ulam {

enum class TypeOp {
    None,
    // const
    MinOf,
    MaxOf,
    SizeOf,
    LengthOf,
    ClassIdOf,
    ConstantOf,
    PositionOf,
    // op
    AtomOf,
    InstanceOf
};

}
