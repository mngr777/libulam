#if !defined(OP)
#    define OP(str, op)
#    error "OP() macro is not defined"
#endif

OP("<noop>", None)
OP("minof", MinOf)
OP("maxof", MaxOf)
OP("sizeof", SizeOf)
OP("lengthof", LengthOf)
OP("classidof", ClassIdOf)
OP("constantof", ConstantOf)
OP("positionof", PositionOf)
OP("atomof", AtomOf)
OP("instanceof", InstanceOf)
