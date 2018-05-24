
/**
Ok, dealing with normal sources

maybe just invalidate the cache when we have a change? That sounds reasonable.

*/

exception Found(Location.t, Types.type_expr);

let showLoc = ({Location.loc_start, loc_end}) => {
  open Lexing;
  Printf.sprintf(
    "%d:%d - %d:%d",
    loc_start.pos_lnum, loc_start.pos_cnum - loc_start.pos_bol,
    loc_end.pos_lnum, loc_end.pos_cnum - loc_end.pos_bol
  )
};

/* TODO there's a much faster way to do this, if we write our own iterator that skips things that are out of bounds */
module F = (Collector: {let check: (Location.t, Types.type_expr) => unit}) => {
  open Typedtree;
  include TypedtreeIter.DefaultIteratorArgument;
  let leave_pattern = pat => {
    Collector.check(pat.pat_loc, pat.pat_type)
  };
  let leave_expression = expr => {
    Collector.check(expr.Typedtree.exp_loc, expr.Typedtree.exp_type)
  };
};

let checkPos = ((line, char), {Location.loc_start: {pos_lnum, pos_bol, pos_cnum}, loc_end} as loc, exp) => {
  open Lexing;
  if (line < pos_lnum || (line == pos_lnum && char < pos_cnum - pos_bol)) {
    ()
  } else if (line > loc_end.pos_lnum || (line == loc_end.pos_lnum && char > loc_end.pos_cnum - loc_end.pos_bol)) {
    ()
  } else {
    raise(Found(loc, exp))
  }
};

let typeAtPos = (pos, cmt) => {
  let module IterIter = TypedtreeIter.MakeIterator(F({
    let check = checkPos(pos);
  }));
  let iter_part = part => switch part {
  | Cmt_format.Partial_structure(str) => IterIter.iter_structure(str)
  | Partial_structure_item(str) => IterIter.iter_structure_item(str)
  | Partial_signature(str) => IterIter.iter_signature(str)
  | Partial_signature_item(str) => IterIter.iter_signature_item(str)
  | Partial_expression(expression) => IterIter.iter_expression(expression)
  | Partial_pattern(pattern) => IterIter.iter_pattern(pattern)
  | Partial_class_expr(class_expr) => IterIter.iter_class_expr(class_expr)
  | Partial_module_type(module_type) => IterIter.iter_module_type(module_type)
  };
  try {switch cmt {
  | Cmt_format.Implementation(str) => {
    IterIter.iter_structure(str);
  }
  | Cmt_format.Interface(sign) => {
    IterIter.iter_signature(sign);
  }
  | Cmt_format.Partial_implementation(parts)
  | Cmt_format.Partial_interface(parts) => {
    Array.iter(iter_part, parts);
  }
  | _ => failwith("Not a valid cmt file")
  }; None} {
    | Found(loc, expr) => Some((loc, expr))
  }
};

open Result;
let getHover = (uri, line, character, state) => {
  let result = State.getCompilationResult(uri, state);
  let result = switch result {
  | AsYouType.ParseError(text) => Error("Cannot hover -- parser error: " ++ text)
  | TypeError(_, cmt) => Ok(cmt)
  | Success(_, cmt) => Ok(cmt)
  };
  switch result {
  | Error(t) => Some((t, Location.none))
  | Ok(cmt) => {
    switch (typeAtPos((line + 1, character), cmt.Cmt_format.cmt_annots)) {
    | None => None
    | Some((loc, expr)) => Some((PrintType.default.expr(PrintType.default, expr) |> PrintType.prettyString, loc))
    }
  }
  }
};