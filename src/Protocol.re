
open Rpc.J;

let pos = (~line, ~character) => o([("line", i(line)), ("character", i(character))]);

let range = (~start, ~end_) => o([("start", start), ("end", end_)]);

open Infix;
let getTextDocument = doc => Json.get("uri", doc) |?> Json.string
    |?> uri => Json.get("version", doc) |?> Json.number
    |?> version => Json.get("text", doc) |?> Json.string
    |?>> text => (uri, version, text);

let getPosition = pos => Json.get("line", pos) |?> Json.number
|?> line => Json.get("character", pos) |?> Json.number
|?>> character => (int_of_float(line), int_of_float(character));

let rgetPosition = pos => (Result.InfixResult.(RJson.get("line", pos) |?> RJson.number
|?> line => RJson.get("character", pos) |?> RJson.number
|?>> character => (int_of_float(line), int_of_float(character))));

let posOfLexing = ({Lexing.pos_lnum, pos_cnum, pos_bol}) => o([
    ("line", i(pos_lnum - 1)),
    ("character", i(pos_cnum - pos_bol))
]);

let rangeOfLoc = ({Location.loc_start, loc_end}) => o([
    ("start", posOfLexing(loc_start)),
    ("end", posOfLexing(loc_end))
]);

let rangeOfInts = (l0, c0, l1, c1) => o([
    ("start", pos(~line=l0, ~character=c0)),
    ("end", pos(~line=l1, ~character=c1)),
]);