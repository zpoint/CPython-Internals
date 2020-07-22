void pr_expr_ty(expr_ty value)
{
    printf("value->kind: %d\n", value->kind);
    PyObject *op = value->v.Constant.value;
    printf("value->v.Constant.value: %s\n", PyUnicode_AsUTF8(op->ob_type->tp_repr(op)));
}

void pr_asdl_seq(asdl_seq* target)
{
    printf("target->size: %llu\n", target->size);
    for (size_t i = 0; i < target->size; ++i)
    {
        pr_expr_ty((expr_ty)target->elements[i]);
    }

}

void pr_ast(stmt_ty s)
{
    printf("in pr_ast, kind: %d\n", s->kind);
    switch (s->kind)
    {
        case Assign_kind:
            pr_asdl_seq(s->v.Assign.targets);
            pr_expr_ty(s->v.Assign.value);
        default:
            printf("unknown kind\n");

    }
}
