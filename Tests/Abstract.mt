
Needs["CodeParser`"]

agg =
	ContainerNode[Box, {
		TernaryNode[TernaryOptionalPattern, {
			LeafNode[Symbol, "a", <|Source -> {1, 1}|>],
			LeafNode[Token`Colon, ":", <|Source -> {1, 2}|>],
			LeafNode[Symbol, "b", <|Source -> {1, 3}|>],
			LeafNode[Token`Colon, ":", <|Source -> {1, 4}|>],
			LeafNode[Symbol, "c", <|Source -> {1, 5}|>]}, <|Source -> {}|>]}, <||>]


(*
bug 409304
*)
Test[
	CodeParser`Abstract`Abstract[agg]
	,
	ContainerNode[Box, {
		CallNode[
			LeafNode[Symbol, "Optional", <||>], {
			CallNode[LeafNode[Symbol, "Pattern", <||>], {
				LeafNode[Symbol, "a", <|Source -> {1, 1}|>],
				LeafNode[Symbol, "b", <|Source -> {1, 3}|>]}, <||>],
			LeafNode[Symbol, "c", <|Source -> {1, 5}|>]}, <|Source -> {}|>]}, <||>]
	,
	TestID->"Abstract-20210430-D1U9S1"
]






(*
Bug 409472
*)
cst = CodeConcreteParseBox[{RowBox[{"Begin", "[", "\"\<FindMinimumTrek`\>\"", "]"}], "\n", RowBox[{"End", "[", "]"}]}]

agg = CodeParser`Abstract`Aggregate[cst]

Test[
	CodeParser`Abstract`Abstract[agg]
	,
	ContainerNode[Box, {
		ContextNode[{LeafNode[String, "\"FindMinimumTrek`\"", <|Source -> {1, 1, 3}|>]}, {}, <|Source -> {1} ;; {3}|>]}, <||>]
	,
	TestID->"Abstract-20210504-G2K4C0"
]






