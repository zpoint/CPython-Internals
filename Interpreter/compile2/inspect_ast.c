void pr_expr_ty(expr_ty value)
{
    printf("value->kind: %d\n", value->kind);
    PyObject *op = NULL;
    switch (value->kind)
    {
        case Constant_kind:
            op = value->v.Constant.value;
            printf("value->v.Constant.value: %s\n", PyUnicode_AsUTF8(op->ob_type->tp_repr(op)));
            break;
        case Name_kind:
            op = value->v.Name.id;
            printf("identifier: %s, _expr_context: %d\n", PyUnicode_AsUTF8(op->ob_type->tp_repr(op)), value->v.Name.ctx);
            break;
        case BinOp_kind:
            printf("left:\n");
            pr_expr_ty(value->v.BinOp.left);
            printf("operator: %d\n", value->v.BinOp.op);
            printf("right:\n");
            pr_expr_ty(value->v.BinOp.right);
            break;
        default:
            printf("unknown kind\n");
    }

}

void pr_asdl_seq(asdl_seq* target)
{
    printf("target->size: %zd\n", target->size);
    for (Py_ssize_t i = 0; i < target->size; ++i)
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
            break;
        default:
            printf("unknown kind\n");
    }
}